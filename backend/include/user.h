#pragma once
#define WIN32_LEAN_AND_MEAN
#include <string>
#include <vector>
#include <cstdint>

#pragma pack(push, 1)
struct UserRecord {
    uint32_t id;
    char     username[64];
    char     email[128];
    char     password_hash[64];
    char     salt[32];
    char     token[64];
    uint64_t created_at;
    uint8_t  active;
    uint8_t  _pad[3];
};
#pragma pack(pop)

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

class UserDatabase {
public:
    explicit UserDatabase(const std::string& filepath);

    bool addUser(User& user);
    bool findByUsername(const std::string& username, User& out) const;
    bool findByToken(const std::string& token, User& out) const;
    bool updateToken(uint32_t id, const std::string& token);
    bool removeUser(const std::string& username);

    std::vector<User> filterByEmail(const std::string& domain) const;
    std::vector<User> filterActive() const;
    std::vector<User> filterByCreatedAfter(uint64_t timestamp) const;

    std::vector<User> sortByUsername()  const;
    std::vector<User> sortByCreatedAt() const;

    size_t   countActive() const;
    uint64_t earliestRegistration() const;

    std::vector<User> getAll() const;

private:
    std::string      filepath_;
    mutable uint32_t nextId_ = 1;

    std::vector<User> loadAll() const;
    void              saveAll(const std::vector<User>& users) const;
    uint32_t          computeNextId() const;
};
