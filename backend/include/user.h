#pragma once
#define WIN32_LEAN_AND_MEAN
#include <string>
#include <vector>
#include <cstdint>

// -------------------------------------------------------------------------
// Binary record layout written to users.bin.
// Total size: 4+64+128+64+32+64+8+1+3 = 368 bytes per record.
// -------------------------------------------------------------------------
#pragma pack(push, 1)
struct UserRecord {
    uint32_t id;             ///< Unique auto-increment ID
    char     username[64];   ///< Null-terminated username
    char     email[128];     ///< Null-terminated email
    char     password_hash[64]; ///< 16-char FNV hex hash
    char     salt[32];       ///< 16-char hex random salt
    char     token[64];      ///< 32-char hex session token (empty if logged out)
    uint64_t created_at;     ///< Unix timestamp (seconds)
    uint8_t  active;         ///< 1 = active, 0 = soft-deleted
    uint8_t  _pad[3];        ///< Alignment padding
};
#pragma pack(pop)

// -------------------------------------------------------------------------
// In-memory representation
// -------------------------------------------------------------------------
class User {
public:
    uint32_t    id          = 0;
    std::string username;
    std::string email;
    std::string password_hash;
    std::string salt;
    std::string token;
    uint64_t    created_at  = 0;
    bool        active      = true;

    User() = default;
    explicit User(const UserRecord& rec);
    UserRecord toRecord() const;
};

// -------------------------------------------------------------------------
// Binary file database for User objects
// -------------------------------------------------------------------------
class UserDatabase {
public:
    explicit UserDatabase(const std::string& filepath);

    /// Add a new user; sets user.id on success. Returns false if username exists.
    bool addUser(User& user);

    /// Find by username (case-sensitive). Returns false if not found/inactive.
    bool findByUsername(const std::string& username, User& out) const;

    /// Find by session token. Returns false if not found.
    bool findByToken(const std::string& token, User& out) const;

    /// Overwrite the token field for the given user ID.
    bool updateToken(uint32_t id, const std::string& token);

    /// Soft-delete: marks the record as inactive.
    bool removeUser(const std::string& username);

    // --- Filtering ---
    std::vector<User> filterByEmail(const std::string& domain) const;
    std::vector<User> filterActive() const;
    std::vector<User> filterByCreatedAfter(uint64_t timestamp) const;

    // --- Sorting ---
    std::vector<User> sortByUsername()  const;
    std::vector<User> sortByCreatedAt() const;

    // --- Summary ---
    size_t   countActive() const;
    uint64_t earliestRegistration() const;

    std::vector<User> getAll() const;

private:
    std::string           filepath_;
    mutable uint32_t      nextId_ = 1;

    std::vector<User> loadAll() const;
    void              saveAll(const std::vector<User>& users) const;
    uint32_t          computeNextId() const;
};
