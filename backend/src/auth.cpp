#include "auth.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>   // for GetTickCount64, QueryPerformanceCounter
#include <ctime>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace auth {

// ---- FNV-1a 64-bit hash -----------------------------------------------

static uint64_t fnv1a64(const std::string& data) {
    uint64_t hash = 14695981039346656037ULL;
    for (unsigned char c : data) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

static std::string toHex16(uint64_t v) {
    char buf[17];
    std::snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(v));
    return std::string(buf);
}

// ---- Public API -------------------------------------------------------

std::string hashPassword(const std::string& password, const std::string& salt) {
    // Double-round: hash(salt+password) then hash(result+salt)
    std::string first  = toHex16(fnv1a64(salt + password));
    std::string second = toHex16(fnv1a64(first + salt));
    return second; // 16-char hex
}

std::string generateSalt() {
    // Mix several entropy sources for a course-project salt
    LARGE_INTEGER pc;
    QueryPerformanceCounter(&pc);
    uint64_t t = static_cast<uint64_t>(std::time(nullptr));
    uint64_t tick = GetTickCount64();
    uint64_t mix = fnv1a64(toHex16(t) + toHex16(tick) +
                           toHex16(static_cast<uint64_t>(pc.QuadPart)));
    return toHex16(mix);   // 16-char hex
}

std::string generateToken(const std::string& username) {
    LARGE_INTEGER pc;
    QueryPerformanceCounter(&pc);
    uint64_t t    = static_cast<uint64_t>(std::time(nullptr));
    uint64_t tick = GetTickCount64();
    uint64_t h1   = fnv1a64(username + toHex16(t));
    uint64_t h2   = fnv1a64(toHex16(tick) +
                            toHex16(static_cast<uint64_t>(pc.QuadPart)) + username);
    return toHex16(h1) + toHex16(h2);  // 32-char hex
}

bool verifyPassword(const std::string& password,
                    const std::string& storedHash,
                    const std::string& salt) {
    return hashPassword(password, salt) == storedHash;
}

} // namespace auth
