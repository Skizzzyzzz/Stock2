#pragma once
#include <string>
#include <vector>
#include <cstdint>

#pragma pack(push, 1)
struct StockRecord {
    char     symbol[16];
    char     name[128];
    double   price;
    double   change_pct;
    uint64_t volume;
    double   market_cap;
    char     signal[16];
    uint64_t last_updated;
};
#pragma pack(pop)

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

    static std::string computeSignal(double change_pct);
};

class StockDatabase {
public:
    explicit StockDatabase(const std::string& filepath);

    void upsert(const Stock& stock);
    bool findBySymbol(const std::string& symbol, Stock& out) const;
    bool removeBySymbol(const std::string& symbol);

    std::vector<Stock> filterBySignal   (const std::string& signal) const;
    std::vector<Stock> filterByMinPrice (double minPrice)            const;
    std::vector<Stock> filterByMaxPrice (double maxPrice)            const;
    std::vector<Stock> filterByMinVolume(uint64_t minVolume)         const;

    std::vector<Stock> sortByPrice (bool ascending = true)  const;
    std::vector<Stock> sortByChange(bool ascending = false) const;
    std::vector<Stock> sortByVolume(bool ascending = false) const;

    size_t countBySignal  (const std::string& signal) const;
    double averagePrice   ()                           const;
    double totalMarketCap ()                           const;

    std::vector<Stock> getAll() const;

private:
    std::string        filepath_;

    std::vector<Stock> loadAll() const;
    void               saveAll(const std::vector<Stock>& stocks) const;
};
