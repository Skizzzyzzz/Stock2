#pragma once
#include <string>
#include <cstdint>

/**
 * Authentication helpers: password hashing with per-user salt,
 * and session token generation.
 *
 * Hashing uses FNV-1a 64-bit with a random 8-byte salt stored as a 16-char
 * hex string.  This is intentionally lightweight for a course project.
 * Tokens are 32-char hex strings derived from the current timestamp and the
 * username hash.
 */
namespace auth {

/**
 * Hash password using FNV-1a 64-bit with the provided salt.
 * @return 16-char lowercase hex string
 */
std::string hashPassword(const std::string& password, const std::string& salt);

/**
 * Generate a random 16-char hex salt.
 */
std::string generateSalt();

/**
 * Generate a unique session token (32-char hex).
 */
std::string generateToken(const std::string& username);

/**
 * Return true if hash(password, salt) == storedHash.
 */
bool verifyPassword(const std::string& password,
                    const std::string& storedHash,
                    const std::string& salt);

} // namespace auth
