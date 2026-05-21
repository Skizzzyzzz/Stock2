#pragma once
#include <string>
#include <vector>
#include <cstdint>

#pragma pack(push, 1)
struct WatchlistRecord {
    uint32_t id;
    uint32_t user_id;
    char     symbol[16];
    char     notes[256];
    uint64_t added_at;
    uint8_t  active;
    uint8_t  _pad[3];
};
#pragma pack(pop)

class WatchlistEntry {
public:
    uint32_t    id       = 0;
    uint32_t    user_id  = 0;
    std::string symbol;
    std::string notes;
    uint64_t    added_at = 0;
    bool        active   = true;

    WatchlistEntry() = default;
    explicit WatchlistEntry(const WatchlistRecord& rec);
    WatchlistRecord toRecord() const;
};

class WatchlistDatabase {
public:
    explicit WatchlistDatabase(const std::string& filepath);

    bool add(WatchlistEntry& entry);
    bool remove(uint32_t user_id, const std::string& symbol);

    std::vector<WatchlistEntry> getByUser(uint32_t user_id) const;
    bool exists(uint32_t user_id, const std::string& symbol) const;

    std::vector<WatchlistEntry> filterBySymbol    (const std::string& symbol) const;
    std::vector<WatchlistEntry> filterByAddedAfter(uint64_t timestamp)        const;

    std::vector<WatchlistEntry> sortByAddedAt(uint32_t user_id, bool ascending = false) const;
    std::vector<WatchlistEntry> sortBySymbol (uint32_t user_id)                         const;

    size_t countByUser (uint32_t user_id) const;
    size_t totalEntries()                 const;

    std::vector<WatchlistEntry> getAll() const;

private:
    std::string      filepath_;
    mutable uint32_t nextId_ = 1;

    std::vector<WatchlistEntry> loadAll() const;
    void                        saveAll(const std::vector<WatchlistEntry>& entries) const;
    uint32_t                    computeNextId() const;
};
