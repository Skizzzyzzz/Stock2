#pragma once
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <map>
#include <vector>
#include <functional>

struct HttpRequest {
    std::string method;
    std::string path;
    std::string query;
    std::map<std::string, std::string> headers;
    std::string body;
};

struct HttpResponse {
    int         status      = 200;
    std::string contentType = "application/json";
    std::string body;

    static HttpResponse json(const std::string& body);
    static HttpResponse notFound(const std::string& msg = "Not found");
    static HttpResponse badRequest(const std::string& msg = "Bad request");
    static HttpResponse serverError(const std::string& msg = "Internal error");
    static HttpResponse cors();
};

using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

struct Route {
    std::string  method;
    std::string  pathPrefix;
    RouteHandler handler;
};

class HttpServer {
public:
    explicit HttpServer(uint16_t port);
    ~HttpServer();

    void addRoute(const std::string& method,
                  const std::string& pathPrefix,
                  RouteHandler       handler);
    void start();

private:
    uint16_t           port_;
    SOCKET             listenSocket_ = INVALID_SOCKET;
    std::vector<Route> routes_;

    void               handleClient(SOCKET clientSocket) const;
    static bool        parseRequest(const std::string& raw, HttpRequest& req);
    static std::string serialiseResponse(const HttpResponse& res);
    const Route*       findRoute(const std::string& method,
                                 const std::string& path) const;
};
