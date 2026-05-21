#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include "server.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <stdexcept>

static const std::string kCorsHeaders =
    "Access-Control-Allow-Origin: *\r\n"
    "Access-Control-Allow-Methods: GET, POST, DELETE, OPTIONS\r\n"
    "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
    "Cache-Control: no-store, no-cache, must-revalidate\r\n"
    "Pragma: no-cache\r\n";

HttpResponse HttpResponse::json(const std::string& body) {
    HttpResponse r;
    r.status      = 200;
    r.contentType = "application/json";
    r.body        = body;
    return r;
}

HttpResponse HttpResponse::notFound(const std::string& msg) {
    HttpResponse r;
    r.status      = 404;
    r.contentType = "application/json";
    r.body        = "{\"error\":\"" + msg + "\"}";
    return r;
}

HttpResponse HttpResponse::badRequest(const std::string& msg) {
    HttpResponse r;
    r.status      = 400;
    r.contentType = "application/json";
    r.body        = "{\"error\":\"" + msg + "\"}";
    return r;
}

HttpResponse HttpResponse::serverError(const std::string& msg) {
    HttpResponse r;
    r.status      = 500;
    r.contentType = "application/json";
    r.body        = "{\"error\":\"" + msg + "\"}";
    return r;
}

HttpResponse HttpResponse::cors() {
    HttpResponse r;
    r.status      = 204;
    r.contentType = "";
    r.body        = "";
    return r;
}

HttpServer::HttpServer(uint16_t port) : port_(port) {}

HttpServer::~HttpServer() {
    if (listenSocket_ != INVALID_SOCKET) closesocket(listenSocket_);
}

void HttpServer::addRoute(const std::string& method,
                          const std::string& pathPrefix,
                          RouteHandler       handler) {
    routes_.push_back({method, pathPrefix, std::move(handler)});
}

const Route* HttpServer::findRoute(const std::string& method,
                                   const std::string& path) const {
    for (auto& r : routes_) {
        if (r.method == "OPTIONS" || r.method == method) {
            if (r.pathPrefix == "*") return &r;
            if (path == r.pathPrefix ||
                (r.pathPrefix.back() == '/' &&
                 path.size() >= r.pathPrefix.size() &&
                 path.substr(0, r.pathPrefix.size()) == r.pathPrefix))
                return &r;
        }
    }
    return nullptr;
}

bool HttpServer::parseRequest(const std::string& raw, HttpRequest& req) {
    size_t pos = 0;
    size_t eol = raw.find("\r\n", pos);
    if (eol == std::string::npos) return false;
    std::string requestLine = raw.substr(pos, eol - pos);
    pos = eol + 2;

    std::istringstream rl(requestLine);
    std::string httpVer;
    rl >> req.method >> req.path >> httpVer;
    if (req.method.empty() || req.path.empty()) return false;

    size_t qmark = req.path.find('?');
    if (qmark != std::string::npos) {
        req.query = req.path.substr(qmark + 1);
        req.path  = req.path.substr(0, qmark);
    }

    size_t contentLength = 0;
    while (pos < raw.size()) {
        eol = raw.find("\r\n", pos);
        if (eol == std::string::npos) break;
        if (eol == pos) { pos += 2; break; }
        std::string line = raw.substr(pos, eol - pos);
        pos = eol + 2;
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string name  = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        size_t vs = value.find_first_not_of(" \t");
        if (vs != std::string::npos) value = value.substr(vs);
        for (char& c : name) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        req.headers[name] = value;
        if (name == "content-length") {
            try { contentLength = std::stoul(value); } catch (...) {}
        }
    }
    if (contentLength > 0 && pos < raw.size())
        req.body = raw.substr(pos, contentLength);
    return true;
}

std::string HttpServer::serialiseResponse(const HttpResponse& res) {
    std::string statusText;
    switch (res.status) {
        case 200: statusText = "OK";                    break;
        case 204: statusText = "No Content";            break;
        case 400: statusText = "Bad Request";           break;
        case 404: statusText = "Not Found";             break;
        case 500: statusText = "Internal Server Error"; break;
        default:  statusText = "OK";
    }
    std::ostringstream oss;
    oss << "HTTP/1.1 " << res.status << " " << statusText << "\r\n";
    oss << kCorsHeaders;
    if (!res.contentType.empty())
        oss << "Content-Type: " << res.contentType << "\r\n";
    oss << "Content-Length: " << res.body.size() << "\r\n";
    oss << "Connection: close\r\n";
    oss << "\r\n";
    oss << res.body;
    return oss.str();
}

void HttpServer::handleClient(SOCKET sock) const {
    std::string raw;
    char buf[4096];
    bool   headersDone  = false;
    size_t contentLength = 0;
    size_t headerEnd    = 0;

    while (true) {
        int n = recv(sock, buf, sizeof(buf) - 1, 0);
        if (n <= 0) break;
        buf[n] = '\0';
        raw.append(buf, n);

        if (!headersDone) {
            size_t he = raw.find("\r\n\r\n");
            if (he != std::string::npos) {
                headersDone = true;
                headerEnd   = he + 4;
                std::string lower = raw.substr(0, headerEnd);
                for (char& c : lower) c = static_cast<char>(
                    std::tolower(static_cast<unsigned char>(c)));
                size_t clPos = lower.find("content-length:");
                if (clPos != std::string::npos) {
                    clPos += 15;
                    while (clPos < lower.size() && lower[clPos] == ' ') ++clPos;
                    size_t clEnd = lower.find("\r\n", clPos);
                    try {
                        contentLength = std::stoul(
                            lower.substr(clPos, clEnd - clPos));
                    } catch (...) {}
                }
            }
        }
        if (headersDone && raw.size() >= headerEnd + contentLength) break;
    }

    HttpRequest req;
    if (!parseRequest(raw, req)) {
        closesocket(sock);
        return;
    }

    HttpResponse res;
    if (req.method == "OPTIONS") {
        res = HttpResponse::cors();
    } else {
        const Route* route = findRoute(req.method, req.path);
        if (route) {
            try {
                res = route->handler(req);
            } catch (const std::exception& ex) {
                std::cerr << "[Server] Handler exception: " << ex.what() << "\n";
                res = HttpResponse::serverError(ex.what());
            }
        } else {
            res = HttpResponse::notFound("No route for " + req.method + " " + req.path);
        }
    }

    std::string responseStr = serialiseResponse(res);
    send(sock, responseStr.c_str(), static_cast<int>(responseStr.size()), 0);
    closesocket(sock);
}

void HttpServer::start() {
    listenSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket_ == INVALID_SOCKET)
        throw std::runtime_error("socket() failed");

    int opt = 1;
    setsockopt(listenSocket_, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket_,
             reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
        throw std::runtime_error("bind() failed on port " + std::to_string(port_));

    if (listen(listenSocket_, SOMAXCONN) == SOCKET_ERROR)
        throw std::runtime_error("listen() failed");

    std::cout << "[Server] Listening on http://localhost:" << port_ << "\n";

    while (true) {
        SOCKET client = accept(listenSocket_, nullptr, nullptr);
        if (client == INVALID_SOCKET) continue;
        std::thread([this, client]() {
            handleClient(client);
        }).detach();
    }
}
