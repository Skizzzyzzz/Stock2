#pragma once
// This file uses WinHTTP — Windows platform only.
// Declared in main.cpp: "Platform: Windows (WinHTTP required)"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include "stock.h"

/**
 * Fetches live stock data from the unofficial Yahoo Finance v7 quote API.
 *
 * Initialisation flow (required once before fetching):
 *   1. GET https://fc.yahoo.com/  — obtain Set-Cookie value
 *   2. GET https://query2.yahoo.com/v1/test/getcrumb — returns crumb string
 *   3. Subsequent requests include Cookie + &crumb= query param
 */
class YahooFinanceClient {
public:
    YahooFinanceClient();
    ~YahooFinanceClient();

    /**
     * Must be called once. Obtains cookies and crumb.
     * Returns true on success; false if Yahoo is unreachable.
     */
    bool init();

    /// Fetch a single quote. Returns Stock with empty symbol on failure.
    Stock fetchQuote(const std::string& symbol);

    /**
     * Fetch multiple quotes in one API call (comma-separated symbols).
     * Skips symbols that Yahoo cannot resolve.
     */
    std::vector<Stock> fetchQuotes(const std::vector<std::string>& symbols);

private:
    HINTERNET session_  = nullptr;
    std::string crumb_;
    std::string cookie_;   ///< Raw Cookie header value to forward

    /**
     * Generic WinHTTP HTTPS GET helper.
     * @param host   Wide-string hostname, e.g. L"query1.finance.yahoo.com"
     * @param path   Wide-string path+query, e.g. L"/v7/finance/quote?symbols=AAPL"
     * @param extraHeaders  Additional request headers (CRLF-separated, may be empty)
     * @return       Response body as UTF-8 string; empty on error
     */
    std::string httpsGet(const std::wstring& host,
                         const std::wstring& path,
                         const std::wstring& extraHeaders = L"") const;

    /// Parse a single stock from a Yahoo Finance JSON result object.
    static Stock parseResult(const std::string& resultJson);
};
