#pragma once
#include <string>
#include <vector>

namespace json {

std::string extractString(const std::string& json, const std::string& key);
double      extractDouble(const std::string& json, const std::string& key);
long long   extractInt   (const std::string& json, const std::string& key);
std::string extractFirstObject(const std::string& json, const std::string& arrayKey);

std::string makeString(const std::string& key, const std::string& val);
std::string makeNumber(const std::string& key, double val);
std::string makeNumber(const std::string& key, long long val);
std::string makeBool  (const std::string& key, bool val);
std::string makeObject(const std::vector<std::string>& fields);
std::string makeArray (const std::vector<std::string>& items);
std::string escapeString(const std::string& s);

}
