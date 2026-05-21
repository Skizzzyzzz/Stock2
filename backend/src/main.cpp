#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include "server.h"
#include "user.h"
#include "stock.h"
#include "watchlist.h"
#include "yahoo_finance.h"
#include "auth.h"
#include "json_utils.h"

#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <mutex>

static const std::vector<std::string> kDefaultSymbols = {
    "AAPL", "MSFT", "GOOGL", "AMZN", "META",
    "TSLA", "NVDA", "JPM",   "V",    "JNJ"
};

static std::string stockToJson(const Stock& s) {
    return json::makeObject({
        json::makeString("symbol", s.symbol),
        json::makeString("name",   s.name),
        json::makeNumber("price",  s.price),
        json::makeNumber("change", s.change_pct),
        json::makeNumber("volume", static_cast<long long>(s.volume)),
        json::makeString("signal", s.signal)
    });
}

static std::string watchlistEntryToJson(const WatchlistEntry& e, const Stock* s) {
    std::vector<std::string> fields = {
        json::makeString("symbol",  e.symbol),
        json::makeString("notes",   e.notes),
        json::makeNumber("addedAt", static_cast<long long>(e.added_at))
    };
    if (s) {
        fields.push_back(json::makeNumber("price",  s->price));
        fields.push_back(json::makeNumber("change", s->change_pct));
        fields.push_back(json::makeString("signal", s->signal));
    }
    return json::makeObject(fields);
}

static std::string tokenFromRequest(const HttpRequest& req) {
    auto it = req.headers.find("authorization");
    if (it == req.headers.end()) return "";
    const std::string& v = it->second;
    if (v.size() > 7 && v.substr(0, 7) == "Bearer ")
        return v.substr(7);
    return v;
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    CreateDirectoryA("data", nullptr);

    UserDatabase      userDb     ("data/users.bin");
    StockDatabase     stockDb    ("data/stocks.bin");
    WatchlistDatabase watchlistDb("data/watchlist.bin");
    YahooFinanceClient yahoo;

    std::cout << "[Main] Initialising Yahoo Finance client...\n";
    if (!yahoo.init())
        std::cerr << "[Main] Yahoo Finance init failed — stock data may be stale.\n";

    HttpServer server(8081);

    static std::vector<Stock> cachedStocks;
    static std::time_t        cacheTime = 0;
    static std::mutex         cacheMutex;
    static const int          CACHE_TTL = 60;

    server.addRoute("GET", "/api/stocks",
        [&](const HttpRequest&) -> HttpResponse {
            std::vector<Stock> stocks;
            {
                std::lock_guard<std::mutex> lock(cacheMutex);
                if (!cachedStocks.empty() &&
                    std::difftime(std::time(nullptr), cacheTime) < CACHE_TTL) {
                    stocks = cachedStocks;
                }
            }
            if (stocks.empty()) {
                stocks = yahoo.fetchQuotes(kDefaultSymbols);
                if (stocks.empty()) {
                    stocks = stockDb.getAll();
                } else {
                    for (auto& s : stocks) stockDb.upsert(s);
                    std::lock_guard<std::mutex> lock(cacheMutex);
                    cachedStocks = stocks;
                    cacheTime    = std::time(nullptr);
                }
            }
            std::vector<std::string> items;
            items.reserve(stocks.size());
            for (auto& s : stocks) items.push_back(stockToJson(s));
            return HttpResponse::json(json::makeArray(items));
        });

    server.addRoute("GET", "/api/stock/",
        [&](const HttpRequest& req) -> HttpResponse {
            std::string symbol = req.path.size() > 11
                               ? req.path.substr(11) : "";
            if (symbol.empty())
                return HttpResponse::badRequest("Missing symbol");

            Stock s = yahoo.fetchQuote(symbol);
            if (s.symbol.empty()) {
                if (!stockDb.findBySymbol(symbol, s))
                    return HttpResponse::notFound("Symbol not found: " + symbol);
            } else {
                stockDb.upsert(s);
            }
            return HttpResponse::json(stockToJson(s));
        });

    server.addRoute("POST", "/api/auth/login",
        [&](const HttpRequest& req) -> HttpResponse {
            std::string username = json::extractString(req.body, "username");
            std::string password = json::extractString(req.body, "password");

            if (username.empty() || password.empty())
                return HttpResponse::json(json::makeObject({
                    json::makeBool  ("success", false),
                    json::makeString("message", "Username and password are required")
                }));

            User user;
            if (!userDb.findByUsername(username, user))
                return HttpResponse::json(json::makeObject({
                    json::makeBool  ("success", false),
                    json::makeString("message", "User not found")
                }));

            if (!auth::verifyPassword(password, user.password_hash, user.salt))
                return HttpResponse::json(json::makeObject({
                    json::makeBool  ("success", false),
                    json::makeString("message", "Incorrect password")
                }));

            std::string token = auth::generateToken(username);
            userDb.updateToken(user.id, token);

            return HttpResponse::json(json::makeObject({
                json::makeBool  ("success",  true),
                json::makeString("token",    token),
                json::makeString("username", username),
                json::makeString("message",  "Login successful")
            }));
        });

    server.addRoute("POST", "/api/auth/register",
        [&](const HttpRequest& req) -> HttpResponse {
            std::string username = json::extractString(req.body, "username");
            std::string email    = json::extractString(req.body, "email");
            std::string password = json::extractString(req.body, "password");

            if (username.empty())
                return HttpResponse::json(json::makeObject({
                    json::makeBool  ("success", false),
                    json::makeString("message", "Username is required")
                }));
            if (username.size() < 3 || username.size() > 32)
                return HttpResponse::json(json::makeObject({
                    json::makeBool  ("success", false),
                    json::makeString("message", "Username must be 3-32 characters")
                }));
            if (password.size() < 6)
                return HttpResponse::json(json::makeObject({
                    json::makeBool  ("success", false),
                    json::makeString("message", "Password must be at least 6 characters")
                }));

            User existing;
            if (userDb.findByUsername(username, existing))
                return HttpResponse::json(json::makeObject({
                    json::makeBool  ("success", false),
                    json::makeString("message", "Username already taken")
                }));

            User newUser;
            newUser.username      = username;
            newUser.email         = email;
            newUser.salt          = auth::generateSalt();
            newUser.password_hash = auth::hashPassword(password, newUser.salt);
            newUser.token         = auth::generateToken(username);
            newUser.created_at    = static_cast<uint64_t>(std::time(nullptr));
            newUser.active        = true;

            if (!userDb.addUser(newUser))
                return HttpResponse::json(json::makeObject({
                    json::makeBool  ("success", false),
                    json::makeString("message", "Registration failed — please retry")
                }));

            return HttpResponse::json(json::makeObject({
                json::makeBool  ("success",  true),
                json::makeString("token",    newUser.token),
                json::makeString("username", username),
                json::makeString("message",  "Registration successful")
            }));
        });

    server.addRoute("GET", "/api/watchlist",
        [&](const HttpRequest& req) -> HttpResponse {
            User user;
            if (!userDb.findByToken(tokenFromRequest(req), user))
                return HttpResponse::json(json::makeObject({
                    json::makeBool  ("success", false),
                    json::makeString("message", "Unauthorized")
                }));

            auto entries = watchlistDb.getByUser(user.id);
            std::vector<std::string> items;
            items.reserve(entries.size());
            for (auto& e : entries) {
                Stock s;
                bool found = stockDb.findBySymbol(e.symbol, s);
                items.push_back(watchlistEntryToJson(e, found ? &s : nullptr));
            }
            return HttpResponse::json(json::makeArray(items));
        });

    server.addRoute("POST", "/api/watchlist",
        [&](const HttpRequest& req) -> HttpResponse {
            User user;
            if (!userDb.findByToken(tokenFromRequest(req), user))
                return HttpResponse::json(json::makeObject({
                    json::makeBool  ("success", false),
                    json::makeString("message", "Unauthorized")
                }));

            std::string symbol = json::extractString(req.body, "symbol");
            std::string notes  = json::extractString(req.body, "notes");
            if (symbol.empty())
                return HttpResponse::badRequest("Symbol required");

            WatchlistEntry entry;
            entry.user_id  = user.id;
            entry.symbol   = symbol;
            entry.notes    = notes;
            entry.added_at = static_cast<uint64_t>(std::time(nullptr));
            entry.active   = true;

            if (!watchlistDb.add(entry))
                return HttpResponse::json(json::makeObject({
                    json::makeBool  ("success", false),
                    json::makeString("message", "Symbol already in watchlist")
                }));

            return HttpResponse::json(json::makeObject({
                json::makeBool  ("success", true),
                json::makeString("message", "Added to watchlist")
            }));
        });

    server.addRoute("DELETE", "/api/watchlist/",
        [&](const HttpRequest& req) -> HttpResponse {
            User user;
            if (!userDb.findByToken(tokenFromRequest(req), user))
                return HttpResponse::json(json::makeObject({
                    json::makeBool  ("success", false),
                    json::makeString("message", "Unauthorized")
                }));

            std::string symbol = req.path.size() > 15
                               ? req.path.substr(15) : "";
            if (symbol.empty())
                return HttpResponse::badRequest("Symbol required");

            bool removed = watchlistDb.remove(user.id, symbol);
            return HttpResponse::json(json::makeObject({
                json::makeBool  ("success", removed),
                json::makeString("message", removed ? "Removed" : "Not found")
            }));
        });

    std::cout << "[Main] Starting server on port 8081...\n";
    server.start();

    WSACleanup();
    return 0;
}
