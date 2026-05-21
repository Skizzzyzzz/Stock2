#include "watchlist.h"
#include <fstream>
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <cctype>

// ---- WatchlistEntry -----------------------------------------------------

WatchlistEntry::WatchlistEntry(const WatchlistRecord& r)
    : id(r.id)
    , user_id(r.user_id)
    , symbol(r.symbol)
    , notes(r.notes)
    , added_at(r.added_at)
    , active(r.active != 0)
{}

WatchlistRecord WatchlistEntry::toRecord() const {
    WatchlistRecord r{};
    r.id      = id;
    r.user_id = user_id;
    std::strncpy(r.symbol, symbol.c_str(), sizeof(r.symbol) - 1);
    std::strncpy(r.notes,  notes.c_str(),  sizeof(r.notes)  - 1);
    r.added_at = added_at;
    r.active   = active ? 1 : 0;
    return r;
}

// ---- WatchlistDatabase --------------------------------------------------

WatchlistDatabase::WatchlistDatabase(const std::string& filepath)
    : filepath_(filepath)
{
    nextId_ = computeNextId();
}

uint32_t WatchlistDatabase::computeNextId() const {
    uint32_t max = 0;
    for (auto& e : loadAll()) if (e.id > max) max = e.id;
    return max + 1;
}

std::vector<WatchlistEntry> WatchlistDatabase::loadAll() const {
    std::vector<WatchlistEntry> entries;
    std::ifstream f(filepath_, std::ios::binary);
    if (!f) return entries;
    WatchlistRecord rec;
    while (f.read(reinterpret_cast<char*>(&rec), sizeof(rec)))
        entries.emplace_back(rec);
    return entries;
}

void WatchlistDatabase::saveAll(const std::vector<WatchlistEntry>& entries) const {
    std::ofstream f(filepath_, std::ios::binary | std::ios::trunc);
    if (!f) throw std::runtime_error("Cannot write " + filepath_);
    for (auto& e : entries) {
        WatchlistRecord rec = e.toRecord();
        f.write(reinterpret_cast<const char*>(&rec), sizeof(rec));
    }
}

static std::string toUpperW(std::string s) {
    for (char& c : s)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

bool WatchlistDatabase::add(WatchlistEntry& entry) {
    if (exists(entry.user_id, entry.symbol)) return false;
    auto all = loadAll();
    entry.id = nextId_++;
    all.push_back(entry);
    saveAll(all);
    return true;
}

bool WatchlistDatabase::remove(uint32_t user_id, const std::string& symbol) {
    auto all = loadAll();
    std::string sym = toUpperW(symbol);
    for (auto& e : all) {
        if (e.user_id == user_id && toUpperW(e.symbol) == sym && e.active) {
            e.active = false;
            saveAll(all);
            return true;
        }
    }
    return false;
}

std::vector<WatchlistEntry> WatchlistDatabase::getByUser(uint32_t user_id) const {
    std::vector<WatchlistEntry> out;
    for (auto& e : loadAll())
        if (e.active && e.user_id == user_id) out.push_back(e);
    return out;
}

bool WatchlistDatabase::exists(uint32_t user_id, const std::string& symbol) const {
    std::string sym = toUpperW(symbol);
    for (auto& e : loadAll())
        if (e.active && e.user_id == user_id && toUpperW(e.symbol) == sym) return true;
    return false;
}

// ---- Filtering ----------------------------------------------------------

std::vector<WatchlistEntry>
WatchlistDatabase::filterBySymbol(const std::string& symbol) const {
    std::vector<WatchlistEntry> out;
    std::string sym = toUpperW(symbol);
    for (auto& e : loadAll())
        if (e.active && toUpperW(e.symbol) == sym) out.push_back(e);
    return out;
}

std::vector<WatchlistEntry>
WatchlistDatabase::filterByAddedAfter(uint64_t ts) const {
    std::vector<WatchlistEntry> out;
    for (auto& e : loadAll())
        if (e.active && e.added_at >= ts) out.push_back(e);
    return out;
}

// ---- Sorting ------------------------------------------------------------

std::vector<WatchlistEntry>
WatchlistDatabase::sortByAddedAt(uint32_t user_id, bool ascending) const {
    auto v = getByUser(user_id);
    std::sort(v.begin(), v.end(),
        [ascending](const WatchlistEntry& a, const WatchlistEntry& b){
            return ascending ? a.added_at < b.added_at : a.added_at > b.added_at;
        });
    return v;
}

std::vector<WatchlistEntry>
WatchlistDatabase::sortBySymbol(uint32_t user_id) const {
    auto v = getByUser(user_id);
    std::sort(v.begin(), v.end(),
        [](const WatchlistEntry& a, const WatchlistEntry& b){
            return a.symbol < b.symbol;
        });
    return v;
}

// ---- Summary ------------------------------------------------------------

size_t WatchlistDatabase::countByUser(uint32_t user_id) const {
    size_t n = 0;
    for (auto& e : loadAll()) if (e.active && e.user_id == user_id) ++n;
    return n;
}

size_t WatchlistDatabase::totalEntries() const {
    size_t n = 0;
    for (auto& e : loadAll()) if (e.active) ++n;
    return n;
}

std::vector<WatchlistEntry> WatchlistDatabase::getAll() const { return loadAll(); }
