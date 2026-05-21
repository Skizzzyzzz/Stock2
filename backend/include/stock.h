#pragma once
#include <string>
#include <vector>
#include <cstdint>

// -------------------------------------------------------------------------
// Binary record layout written to stocks.bin.
// Total size: 16+128+8+8+8+8+16+8 = 200 bytes per record.
// -------------------------------------------------------------------------
#pragma pack(push, 1)
struct StockRecord {
    char     symbol[16];    ///< Ticker symbol, null-terminated
    char     name[128];     ///< Company name, null-terminated
    double   price;         ///< Current market price (USD)
    double   change_pct;    ///< Daily change in percent
    uint64_t volume;        ///< Trading volume
    double   market_cap;    ///< Market capitalisation (USD)
    char     signal[16];    ///< Trading signal string
    uint64_t last_updated;  ///< Unix timestamp of last fetch
};
#pragma pack(pop)

// -------------------------------------------------------------------------
// In-memory representation
// -------------------------------------------------------------------------
class Stock {
public:
    std::string symbol;
    std::string name;
    double      price        = 0.0;
    double      change_pct   = 0.0;
    uint64_t    volume       = 0;
    double      market_cap   = 0.0;
    std::string signal;
    uint64_t    last_updated = 0;

    Stock() = default;
    explicit Stock(const StockRecord& rec);
    StockRecord toRecord() const;

    /// Derive a trading signal from change_pct.
    static std::string computeSignal(double change_pct);
};

// -------------------------------------------------------------------------
// Binary file database for Stock objects
// Key is symbol (case-insensitive). Upsert replaces existing record.
// -------------------------------------------------------------------------
class StockDatabase {
public:
    explicit StockDatabase(const std::string& filepath);

    /// Insert or replace a stock record.
    void upsert(const Stock& stock);

    /// Find by symbol. Returns false if not present.
    bool findBySymbol(const std::string& symbol, Stock& out) const;

    /// Remove a stock by symbol. Returns false if not found.
    bool removeBySymbol(const std::string& symbol);

    // --- Filtering (3+ required by spec) ---
    std::vector<Stock> filterBySignal   (const std::string& signal)   const;
    std::vector<Stock> filterByMinPrice (double minPrice)              const;
    std::vector<Stock> filterByMaxPrice (double maxPrice)              const;
    std::vector<Stock> filterByMinVolume(uint64_t minVolume)           const;

    // --- Sorting (2+ required) ---
    std::vector<Stock> sortByPrice (bool ascending = true)  const;
    std::vector<Stock> sortByChange(bool ascending = false) const;
    std::vector<Stock> sortByVolume(bool ascending = false) const;

    // --- Summary (2+ required) ---
    size_t countBySignal  (const std::string& signal) const;
    double averagePrice   ()                           const;
    double totalMarketCap ()                           const;

    std::vector<Stock> getAll() const;

private:
    std::string        filepath_;

    std::vector<Stock> loadAll() const;
    void               saveAll(const std::vector<Stock>& stocks) const;
};
