#include "stock.h"
#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <cctype>

Stock::Stock(const StockRecord& r)
    : symbol(r.symbol)
    , name(r.name)
    , price(r.price)
    , change_pct(r.change_pct)
    , volume(r.volume)
    , market_cap(r.market_cap)
    , signal(r.signal)
    , last_updated(r.last_updated)
{}

StockRecord Stock::toRecord() const {
    StockRecord r{};
    std::snprintf(r.symbol, sizeof(r.symbol), "%s", symbol.c_str());
    std::snprintf(r.name,   sizeof(r.name),   "%s", name.c_str());
    r.price        = price;
    r.change_pct   = change_pct;
    r.volume       = volume;
    r.market_cap   = market_cap;
    std::snprintf(r.signal, sizeof(r.signal), "%s", signal.c_str());
    r.last_updated = last_updated;
    return r;
}

std::string Stock::computeSignal(double change_pct) {
    if (change_pct >= 3.0)  return "STRONG BUY";
    if (change_pct >= 1.0)  return "BUY";
    if (change_pct <= -3.0) return "STRONG SELL";
    if (change_pct <= -1.0) return "SELL";
    return "HOLD";
}

StockDatabase::StockDatabase(const std::string& filepath)
    : filepath_(filepath)
{}

std::vector<Stock> StockDatabase::loadAll() const {
    std::vector<Stock> stocks;
    std::ifstream f(filepath_, std::ios::binary);
    if (!f) return stocks;
    StockRecord rec;
    while (f.read(reinterpret_cast<char*>(&rec), sizeof(rec)))
        stocks.emplace_back(rec);
    return stocks;
}

void StockDatabase::saveAll(const std::vector<Stock>& stocks) const {
    std::ofstream f(filepath_, std::ios::binary | std::ios::trunc);
    if (!f) throw std::runtime_error("Cannot write " + filepath_);
    for (auto& s : stocks) {
        StockRecord rec = s.toRecord();
        f.write(reinterpret_cast<const char*>(&rec), sizeof(rec));
    }
}

static std::string toUpper(std::string s) {
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

void StockDatabase::upsert(const Stock& stock) {
    auto all = loadAll();
    std::string sym = toUpper(stock.symbol);
    for (auto& s : all) {
        if (toUpper(s.symbol) == sym) { s = stock; saveAll(all); return; }
    }
    all.push_back(stock);
    saveAll(all);
}

bool StockDatabase::findBySymbol(const std::string& symbol, Stock& out) const {
    std::string sym = toUpper(symbol);
    for (auto& s : loadAll()) {
        if (toUpper(s.symbol) == sym) { out = s; return true; }
    }
    return false;
}

bool StockDatabase::removeBySymbol(const std::string& symbol) {
    auto all = loadAll();
    std::string sym = toUpper(symbol);
    auto it = std::remove_if(all.begin(), all.end(),
        [&](const Stock& s){ return toUpper(s.symbol) == sym; });
    if (it == all.end()) return false;
    all.erase(it, all.end());
    saveAll(all);
    return true;
}

std::vector<Stock> StockDatabase::filterBySignal(const std::string& signal) const {
    std::vector<Stock> out;
    for (auto& s : loadAll()) if (s.signal == signal) out.push_back(s);
    return out;
}

std::vector<Stock> StockDatabase::filterByMinPrice(double minPrice) const {
    std::vector<Stock> out;
    for (auto& s : loadAll()) if (s.price >= minPrice) out.push_back(s);
    return out;
}

std::vector<Stock> StockDatabase::filterByMaxPrice(double maxPrice) const {
    std::vector<Stock> out;
    for (auto& s : loadAll()) if (s.price <= maxPrice) out.push_back(s);
    return out;
}

std::vector<Stock> StockDatabase::filterByMinVolume(uint64_t minVolume) const {
    std::vector<Stock> out;
    for (auto& s : loadAll()) if (s.volume >= minVolume) out.push_back(s);
    return out;
}

std::vector<Stock> StockDatabase::sortByPrice(bool ascending) const {
    auto v = loadAll();
    std::sort(v.begin(), v.end(), [ascending](const Stock& a, const Stock& b){
        return ascending ? a.price < b.price : a.price > b.price;
    });
    return v;
}

std::vector<Stock> StockDatabase::sortByChange(bool ascending) const {
    auto v = loadAll();
    std::sort(v.begin(), v.end(), [ascending](const Stock& a, const Stock& b){
        return ascending ? a.change_pct < b.change_pct : a.change_pct > b.change_pct;
    });
    return v;
}

std::vector<Stock> StockDatabase::sortByVolume(bool ascending) const {
    auto v = loadAll();
    std::sort(v.begin(), v.end(), [ascending](const Stock& a, const Stock& b){
        return ascending ? a.volume < b.volume : a.volume > b.volume;
    });
    return v;
}

size_t StockDatabase::countBySignal(const std::string& signal) const {
    size_t n = 0;
    for (auto& s : loadAll()) if (s.signal == signal) ++n;
    return n;
}

double StockDatabase::averagePrice() const {
    auto all = loadAll();
    if (all.empty()) return 0.0;
    double sum = 0.0;
    for (auto& s : all) sum += s.price;
    return sum / static_cast<double>(all.size());
}

double StockDatabase::totalMarketCap() const {
    double total = 0.0;
    for (auto& s : loadAll()) total += s.market_cap;
    return total;
}

std::vector<Stock> StockDatabase::getAll() const { return loadAll(); }
