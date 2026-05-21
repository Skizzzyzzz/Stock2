#pragma once
#include <string>
#include <vector>

/**
 * Lightweight JSON extraction and building utilities.
 * Extraction uses string search — suitable for Yahoo Finance flat responses.
 * Building assembles RFC 8259 compliant JSON strings.
 */
namespace json {

// --- Extraction ---
std::string extractString(const std::string& json, const std::string& key);
double      extractDouble(const std::string& json, const std::string& key);
long long   extractInt   (const std::string& json, const std::string& key);
// Returns the raw text of the first object inside a named JSON array.
std::string extractFirstObject(const std::string& json, const std::string& arrayKey);

// --- Building ---
std::string makeString(const std::string& key, const std::string& val);
std::string makeNumber(const std::string& key, double val);
std::string makeNumber(const std::string& key, long long val);   // overload
std::string makeBool  (const std::string& key, bool val);
// Joins field strings into {"k":v,...}
std::string makeObject(const std::vector<std::string>& fields);
// Joins item strings into [item,...]
std::string makeArray (const std::vector<std::string>& items);

// Escape a raw string for safe JSON embedding
std::string escapeString(const std::string& s);

} // namespace json
