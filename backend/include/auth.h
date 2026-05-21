#pragma once
#include <string>
#include <cstdint>

namespace auth {

std::string hashPassword(const std::string& password, const std::string& salt);
std::string generateSalt();
std::string generateToken(const std::string& username);
bool        verifyPassword(const std::string& password,
                           const std::string& storedHash,
                           const std::string& salt);
}
