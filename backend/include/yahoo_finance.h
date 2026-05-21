#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include "stock.h"

class YahooFinanceClient {
public:
    YahooFinanceClient();
    ~YahooFinanceClient();

    bool init();
    Stock fetchQuote(const std::string& symbol);
    std::vector<Stock> fetchQuotes(const std::vector<std::string>& symbols);

private:
    HINTERNET   session_  = nullptr;
    std::string crumb_;
    std::string cookie_;

    std::string httpsGet(const std::wstring& host,
                         const std::wstring& path,
                         const std::wstring& extraHeaders = L"") const;

    static Stock parseResult(const std::string& resultJson);
};
