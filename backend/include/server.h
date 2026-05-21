#pragma once
// Windows-only: WinSock2 HTTP server
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <map>
#include <vector>
#include <functional>

struct HttpRequest {
    std::string method;
    std::string path;          ///< URL path only (no query string)
    std::string query;         ///< Raw query string (after '?')
    std::map<std::string, std::string> headers;
    std::string body;
};

struct HttpResponse {
    int         status      = 200;
    std::string contentType = "application/json";
    std::string body;

    /// Build a 200 JSON response with CORS headers.
    static HttpResponse json(const std::string& body);
    /// Build a 404 Not Found response.
    static HttpResponse notFound(const std::string& msg = "Not found");
    /// Build a 400 Bad Request response.
    static HttpResponse badRequest(const std::string& msg = "Bad request");
    /// Build a 500 Internal Server Error response.
    static HttpResponse serverError(const std::string& msg = "Internal error");
    /// Empty 204 used for OPTIONS pre-flight.
    static HttpResponse cors();
};

/// Route handler signature
using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

struct Route {
    std::string  method;
    std::string  pathPrefix; ///< Matched with starts-with (not full regex)
    RouteHandler handler;
};

/**
 * Minimal single-threaded HTTP/1.1 server using WinSock2.
 * Spawns one std::thread per accepted connection.
 * Call start() — it blocks until the process is killed.
 *
 * Routes are matched in registration order; first match wins.
 * Path matching uses prefix comparison so "/api/stock/" matches
 * "/api/stock/AAPL".
 */
class HttpServer {
public:
    explicit HttpServer(uint16_t port);
    ~HttpServer();

    /// Register a route handler.
    void addRoute(const std::string& method,
                  const std::string& pathPrefix,
                  RouteHandler       handler);

    /// Start listening; blocks forever.
    void start();

private:
    uint16_t          port_;
    SOCKET            listenSocket_ = INVALID_SOCKET;
    std::vector<Route> routes_;

    /// Called on a newly accepted client socket in its own thread.
    void handleClient(SOCKET clientSocket) const;

    /// Parse raw HTTP bytes into HttpRequest.
    static bool parseRequest(const std::string& raw, HttpRequest& req);

    /// Serialise HttpResponse into an HTTP/1.1 response string.
    static std::string serialiseResponse(const HttpResponse& res);

    /// Look up the first matching route; returns nullptr if none.
    const Route* findRoute(const std::string& method,
                           const std::string& path) const;
};
