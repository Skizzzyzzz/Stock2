#include "user.h"
#include <fstream>
#include <algorithm>
#include <cstring>
#include <stdexcept>

// ---- User ---------------------------------------------------------------

User::User(const UserRecord& r)
    : id(r.id)
    , username(r.username)
    , email(r.email)
    , password_hash(r.password_hash)
    , salt(r.salt)
    , token(r.token)
    , created_at(r.created_at)
    , active(r.active != 0)
{}

UserRecord User::toRecord() const {
    UserRecord r{};
    r.id = id;
    std::strncpy(r.username,      username.c_str(),      sizeof(r.username)      - 1);
    std::strncpy(r.email,         email.c_str(),         sizeof(r.email)         - 1);
    std::strncpy(r.password_hash, password_hash.c_str(), sizeof(r.password_hash) - 1);
    std::strncpy(r.salt,          salt.c_str(),          sizeof(r.salt)          - 1);
    std::strncpy(r.token,         token.c_str(),         sizeof(r.token)         - 1);
    r.created_at = created_at;
    r.active     = active ? 1 : 0;
    return r;
}

// ---- UserDatabase -------------------------------------------------------

UserDatabase::UserDatabase(const std::string& filepath)
    : filepath_(filepath)
{
    nextId_ = computeNextId();
}

uint32_t UserDatabase::computeNextId() const {
    auto all = loadAll();
    uint32_t max = 0;
    for (auto& u : all) if (u.id > max) max = u.id;
    return max + 1;
}

std::vector<User> UserDatabase::loadAll() const {
    std::vector<User> users;
    std::ifstream f(filepath_, std::ios::binary);
    if (!f) return users;
    UserRecord rec;
    while (f.read(reinterpret_cast<char*>(&rec), sizeof(rec)))
        users.emplace_back(rec);
    return users;
}

void UserDatabase::saveAll(const std::vector<User>& users) const {
    std::ofstream f(filepath_, std::ios::binary | std::ios::trunc);
    if (!f) throw std::runtime_error("Cannot write " + filepath_);
    for (auto& u : users) {
        UserRecord rec = u.toRecord();
        f.write(reinterpret_cast<const char*>(&rec), sizeof(rec));
    }
}

bool UserDatabase::addUser(User& user) {
    auto all = loadAll();
    for (auto& u : all)
        if (u.active && u.username == user.username) return false;
    user.id = nextId_++;
    all.push_back(user);
    saveAll(all);
    return true;
}

bool UserDatabase::findByUsername(const std::string& username, User& out) const {
    for (auto& u : loadAll())
        if (u.active && u.username == username) { out = u; return true; }
    return false;
}

bool UserDatabase::findByToken(const std::string& token, User& out) const {
    if (token.empty()) return false;
    for (auto& u : loadAll())
        if (u.active && u.token == token) { out = u; return true; }
    return false;
}

bool UserDatabase::updateToken(uint32_t id, const std::string& token) {
    auto all = loadAll();
    for (auto& u : all) {
        if (u.id == id) { u.token = token; saveAll(all); return true; }
    }
    return false;
}

bool UserDatabase::removeUser(const std::string& username) {
    auto all = loadAll();
    for (auto& u : all) {
        if (u.active && u.username == username) {
            u.active = false; saveAll(all); return true;
        }
    }
    return false;
}

// ---- Filtering ----------------------------------------------------------

std::vector<User> UserDatabase::filterByEmail(const std::string& domain) const {
    std::vector<User> out;
    for (auto& u : loadAll())
        if (u.active && u.email.find(domain) != std::string::npos) out.push_back(u);
    return out;
}

std::vector<User> UserDatabase::filterActive() const {
    std::vector<User> out;
    for (auto& u : loadAll()) if (u.active) out.push_back(u);
    return out;
}

std::vector<User> UserDatabase::filterByCreatedAfter(uint64_t ts) const {
    std::vector<User> out;
    for (auto& u : loadAll())
        if (u.active && u.created_at >= ts) out.push_back(u);
    return out;
}

// ---- Sorting ------------------------------------------------------------

std::vector<User> UserDatabase::sortByUsername() const {
    auto v = filterActive();
    std::sort(v.begin(), v.end(),
              [](const User& a, const User& b){ return a.username < b.username; });
    return v;
}

std::vector<User> UserDatabase::sortByCreatedAt() const {
    auto v = filterActive();
    std::sort(v.begin(), v.end(),
              [](const User& a, const User& b){ return a.created_at < b.created_at; });
    return v;
}

// ---- Summary ------------------------------------------------------------

size_t UserDatabase::countActive() const {
    size_t n = 0;
    for (auto& u : loadAll()) if (u.active) ++n;
    return n;
}

uint64_t UserDatabase::earliestRegistration() const {
    uint64_t earliest = UINT64_MAX;
    for (auto& u : loadAll())
        if (u.active && u.created_at < earliest) earliest = u.created_at;
    return earliest == UINT64_MAX ? 0 : earliest;
}

std::vector<User> UserDatabase::getAll() const { return loadAll(); }
