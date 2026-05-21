// Platform: Windows (WinHTTP required)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

#include "yahoo_finance.h"
#include "json_utils.h"
#include "stock.h"

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <ctime>
#include <algorithm>
#include <cctype>

// ---- Wide-string helpers -----------------------------------------------

static std::wstring toWide(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(),
                                  static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()),
                        &w[0], len);
    return w;
}

static std::string toNarrow(const wchar_t* w, int len) {
    if (!w || len <= 0) return {};
    int bytes = WideCharToMultiByte(CP_UTF8, 0, w, len, nullptr, 0, nullptr, nullptr);
    std::string s(bytes, 0);
    WideCharToMultiByte(CP_UTF8, 0, w, len, &s[0], bytes, nullptr, nullptr);
    return s;
}

// ---- YahooFinanceClient -------------------------------------------------

YahooFinanceClient::YahooFinanceClient() {
    session_ = WinHttpOpen(
        L"Mozilla/5.0 StockAnalyzer/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
}

YahooFinanceClient::~YahooFinanceClient() {
    if (session_) WinHttpCloseHandle(session_);
}

// Generic HTTPS GET; returns response body or "" on failure.
std::string YahooFinanceClient::httpsGet(const std::wstring& host,
                                          const std::wstring& path,
                                          const std::wstring& extraHeaders) const {
    if (!session_) return "";

    HINTERNET hConn = WinHttpConnect(session_, host.c_str(),
                                     INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConn) return "";

    HINTERNET hReq = WinHttpOpenRequest(
        hConn, L"GET", path.c_str(),
        nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);
    if (!hReq) { WinHttpCloseHandle(hConn); return ""; }

    // Common browser-like headers
    std::wstring headers =
        L"Accept: application/json, text/plain, */*\r\n"
        L"Accept-Language: en-US,en;q=0.9\r\n";
    if (!extraHeaders.empty()) headers += extraHeaders;

    BOOL sent = WinHttpSendRequest(
        hReq,
        headers.c_str(), static_cast<DWORD>(headers.size()),
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

    if (!sent || !WinHttpReceiveResponse(hReq, nullptr)) {
        WinHttpCloseHandle(hReq);
        WinHttpCloseHandle(hConn);
        return "";
    }

    // Collect response body
    std::string body;
    DWORD bytesAvail = 0;
    while (WinHttpQueryDataAvailable(hReq, &bytesAvail) && bytesAvail > 0) {
        std::string chunk(bytesAvail, '\0');
        DWORD bytesRead = 0;
        WinHttpReadData(hReq, &chunk[0], bytesAvail, &bytesRead);
        body.append(chunk, 0, bytesRead);
    }

    // Extract Set-Cookie from response headers if we haven't got one yet
    if (cookie_.empty()) {
        DWORD size = 0;
        WinHttpQueryHeaders(hReq,
            WINHTTP_QUERY_SET_COOKIE | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
            WINHTTP_HEADER_NAME_BY_INDEX, nullptr, &size,
            WINHTTP_NO_HEADER_INDEX);
        // The above call fails (it's a response header not request),
        // use WINHTTP_QUERY_RAW_HEADERS_CRLF to grab all response headers.
        size = 0;
        WinHttpQueryHeaders(hReq, WINHTTP_QUERY_RAW_HEADERS_CRLF,
            WINHTTP_HEADER_NAME_BY_INDEX, nullptr, &size, WINHTTP_NO_HEADER_INDEX);
        if (size > 0) {
            std::wstring rawHdrs(size / sizeof(wchar_t), L'\0');
            WinHttpQueryHeaders(hReq, WINHTTP_QUERY_RAW_HEADERS_CRLF,
                WINHTTP_HEADER_NAME_BY_INDEX, &rawHdrs[0], &size,
                WINHTTP_NO_HEADER_INDEX);
            std::string narrow = toNarrow(rawHdrs.c_str(),
                                          static_cast<int>(rawHdrs.size()));
            // Find Set-Cookie: value
            size_t pos = narrow.find("Set-Cookie:");
            if (pos == std::string::npos)
                pos = narrow.find("set-cookie:");
            if (pos != std::string::npos) {
                pos += 11;
                while (pos < narrow.size() && narrow[pos] == ' ') ++pos;
                size_t end = narrow.find("\r\n", pos);
                if (end == std::string::npos) end = narrow.size();
                // Take only name=value (before the first semicolon)
                size_t semi = narrow.find(';', pos);
                if (semi < end) end = semi;
                const_cast<YahooFinanceClient*>(this)->cookie_ =
                    narrow.substr(pos, end - pos);
            }
        }
    }

    WinHttpCloseHandle(hReq);
    WinHttpCloseHandle(hConn);
    return body;
}

bool YahooFinanceClient::init() {
    if (!session_) return false;

    // Step 1: Hit the finance consent/cookie page to obtain a session cookie
    (void)httpsGet(L"fc.yahoo.com", L"/", L"");

    // If we still have no cookie from fc.yahoo.com, try finance.yahoo.com
    if (cookie_.empty())
        (void)httpsGet(L"finance.yahoo.com", L"/", L"");

    // Step 2: Fetch crumb using the cookie we obtained
    std::wstring cookieHeader;
    if (!cookie_.empty())
        cookieHeader = L"Cookie: " + toWide(cookie_) + L"\r\n";

    std::string crumbBody = httpsGet(
        L"query2.finance.yahoo.com",
        L"/v1/test/getcrumb",
        cookieHeader);

    crumb_ = crumbBody;
    // Remove any surrounding whitespace/newlines
    while (!crumb_.empty() && (crumb_.back() == '\n' ||
                                crumb_.back() == '\r' ||
                                crumb_.back() == ' '))
        crumb_.pop_back();

    if (crumb_.empty()) {
        std::cerr << "[Yahoo] Warning: could not obtain crumb; "
                     "requests may fail.\n";
        return false;
    }
    std::cout << "[Yahoo] Initialised. Crumb obtained.\n";
    return true;
}

// ---- Parsing -----------------------------------------------------------

Stock YahooFinanceClient::parseResult(const std::string& obj) {
    Stock s;
    s.symbol     = json::extractString(obj, "symbol");
    s.name       = json::extractString(obj, "shortName");
    if (s.name.empty()) s.name = json::extractString(obj, "longName");
    if (s.name.empty()) s.name = s.symbol;

    s.price      = json::extractDouble(obj, "regularMarketPrice");
    s.change_pct = json::extractDouble(obj, "regularMarketChangePercent");
    s.volume     = static_cast<uint64_t>(json::extractInt(obj, "regularMarketVolume"));
    s.market_cap = json::extractDouble(obj, "marketCap");
    s.last_updated = static_cast<uint64_t>(std::time(nullptr));
    s.signal     = Stock::computeSignal(s.change_pct);
    return s;
}

Stock YahooFinanceClient::fetchQuote(const std::string& symbol) {
    std::string path = "/v7/finance/quote?symbols=" + symbol;
    if (!crumb_.empty()) path += "&crumb=" + crumb_;

    std::wstring extraHeaders;
    if (!cookie_.empty())
        extraHeaders = L"Cookie: " + toWide(cookie_) + L"\r\n";

    std::string body = httpsGet(L"query1.finance.yahoo.com",
                                toWide(path), extraHeaders);
    if (body.empty()) return {};

    std::string obj = json::extractFirstObject(body, "result");
    if (obj.empty()) return {};
    return parseResult(obj);
}

std::vector<Stock> YahooFinanceClient::fetchQuotes(
        const std::vector<std::string>& symbols) {
    if (symbols.empty()) return {};

    // Build comma-separated list
    std::string syms;
    for (size_t i = 0; i < symbols.size(); ++i) {
        if (i) syms += ',';
        syms += symbols[i];
    }

    std::string path = "/v7/finance/quote?symbols=" + syms;
    if (!crumb_.empty()) path += "&crumb=" + crumb_;

    std::wstring extraHeaders;
    if (!cookie_.empty())
        extraHeaders = L"Cookie: " + toWide(cookie_) + L"\r\n";

    std::string body = httpsGet(L"query1.finance.yahoo.com",
                                toWide(path), extraHeaders);
    if (body.empty()) return {};

    // Extract the "result" array and iterate over each object
    std::string search = "\"result\":[";
    size_t pos = body.find(search);
    if (pos == std::string::npos) return {};
    pos += search.size();

    std::vector<Stock> results;
    while (pos < body.size()) {
        // Skip to next '{'
        pos = body.find('{', pos);
        if (pos == std::string::npos) break;
        // Find matching '}'
        size_t start = pos;
        int depth = 0;
        while (pos < body.size()) {
            if (body[pos] == '{') ++depth;
            else if (body[pos] == '}') { --depth; if (depth == 0) { ++pos; break; } }
            else if (body[pos] == '"') {
                ++pos;
                while (pos < body.size() && body[pos] != '"') {
                    if (body[pos] == '\\') ++pos;
                    ++pos;
                }
            }
            ++pos;
        }
        std::string obj = body.substr(start, pos - start);
        Stock s = parseResult(obj);
        if (!s.symbol.empty()) results.push_back(s);

        // Stop at the closing ']'
        size_t next = body.find_first_not_of(" \t\r\n,", pos);
        if (next == std::string::npos || body[next] == ']') break;
    }
    return results;
}
