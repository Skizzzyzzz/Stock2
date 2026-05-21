#pragma once
#include <string>
#include <vector>
#include <cstdint>

// -------------------------------------------------------------------------
// Binary record layout written to watchlist.bin.
// Total size: 4+4+16+256+8+1+3 = 292 bytes per record.
// -------------------------------------------------------------------------
#pragma pack(push, 1)
struct WatchlistRecord {
    uint32_t id;          ///< Auto-increment record ID
    uint32_t user_id;     ///< FK to UserRecord.id
    char     symbol[16];  ///< Stock ticker, null-terminated
    char     notes[256];  ///< Optional user notes, null-terminated
    uint64_t added_at;    ///< Unix timestamp
    uint8_t  active;      ///< 1 = active, 0 = soft-deleted
    uint8_t  _pad[3];
};
#pragma pack(pop)

// -------------------------------------------------------------------------
// In-memory representation
// -------------------------------------------------------------------------
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

// -------------------------------------------------------------------------
// Binary file database for WatchlistEntry objects
// -------------------------------------------------------------------------
class WatchlistDatabase {
public:
    explicit WatchlistDatabase(const std::string& filepath);

    /// Add entry; sets entry.id. Returns false if (user_id, symbol) duplicate.
    bool add(WatchlistEntry& entry);

    /// Soft-delete entry by (user_id, symbol).
    bool remove(uint32_t user_id, const std::string& symbol);

    /// All active entries for a user.
    std::vector<WatchlistEntry> getByUser(uint32_t user_id) const;

    /// True if user already watches symbol.
    bool exists(uint32_t user_id, const std::string& symbol) const;

    // --- Filtering ---
    std::vector<WatchlistEntry> filterBySymbol    (const std::string& symbol)    const;
    std::vector<WatchlistEntry> filterByAddedAfter(uint64_t timestamp)           const;

    // --- Sorting ---
    std::vector<WatchlistEntry> sortByAddedAt(uint32_t user_id, bool ascending = false) const;
    std::vector<WatchlistEntry> sortBySymbol (uint32_t user_id)                         const;

    // --- Summary ---
    size_t countByUser   (uint32_t user_id) const;
    size_t totalEntries  ()                 const;

    std::vector<WatchlistEntry> getAll() const;

private:
    std::string filepath_;
    mutable uint32_t nextId_ = 1;

    std::vector<WatchlistEntry> loadAll() const;
    void                        saveAll(const std::vector<WatchlistEntry>& entries) const;
    uint32_t                    computeNextId() const;
};
