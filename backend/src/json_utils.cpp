#include "json_utils.h"
#include <sstream>
#include <iomanip>

namespace json {

static size_t skipWS(const std::string& s, size_t pos) {
    while (pos < s.size() && (s[pos]==' '||s[pos]=='\t'||s[pos]=='\r'||s[pos]=='\n'))
        ++pos;
    return pos;
}

std::string extractString(const std::string& j, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = j.find(search);
    if (pos == std::string::npos) return "";
    pos = skipWS(j, pos + search.size());
    if (pos >= j.size() || j[pos] != ':') return "";
    pos = skipWS(j, pos + 1);
    if (pos >= j.size() || j[pos] != '"') return "";
    ++pos;
    std::string result;
    while (pos < j.size() && j[pos] != '"') {
        if (j[pos] == '\\' && pos + 1 < j.size()) {
            ++pos;
            switch (j[pos]) {
                case '"':  result += '"';  break;
                case '\\': result += '\\'; break;
                case '/':  result += '/';  break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                default:   result += j[pos]; break;
            }
        } else {
            result += j[pos];
        }
        ++pos;
    }
    return result;
}

double extractDouble(const std::string& j, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = j.find(search);
    if (pos == std::string::npos) return 0.0;
    pos = skipWS(j, pos + search.size());
    if (pos >= j.size() || j[pos] != ':') return 0.0;
    pos = skipWS(j, pos + 1);
    if (j.substr(pos, 4) == "null") return 0.0;
    size_t end = pos;
    while (end < j.size() && (std::isdigit((unsigned char)j[end]) ||
           j[end]=='.'||j[end]=='-'||j[end]=='+'||j[end]=='e'||j[end]=='E'))
        ++end;
    if (end == pos) return 0.0;
    try { return std::stod(j.substr(pos, end - pos)); } catch (...) { return 0.0; }
}

long long extractInt(const std::string& j, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = j.find(search);
    if (pos == std::string::npos) return 0;
    pos = skipWS(j, pos + search.size());
    if (pos >= j.size() || j[pos] != ':') return 0;
    pos = skipWS(j, pos + 1);
    if (j.substr(pos, 4) == "null") return 0;
    size_t end = pos;
    if (end < j.size() && j[end] == '-') ++end;
    while (end < j.size() && std::isdigit((unsigned char)j[end])) ++end;
    if (end == pos) return 0;
    try { return std::stoll(j.substr(pos, end - pos)); } catch (...) { return 0; }
}

std::string extractFirstObject(const std::string& j, const std::string& arrayKey) {
    std::string search = "\"" + arrayKey + "\"";
    size_t pos = j.find(search);
    if (pos == std::string::npos) return "";
    pos = skipWS(j, pos + search.size());
    if (pos >= j.size() || j[pos] != ':') return "";
    pos = skipWS(j, pos + 1);
    if (pos >= j.size() || j[pos] != '[') return "";
    pos = skipWS(j, pos + 1);
    if (pos >= j.size() || j[pos] != '{') return "";
    size_t start = pos;
    int depth = 0;
    while (pos < j.size()) {
        if      (j[pos] == '{') ++depth;
        else if (j[pos] == '}') { --depth; if (depth == 0) { ++pos; break; } }
        else if (j[pos] == '"') {
            ++pos;
            while (pos < j.size() && j[pos] != '"') {
                if (j[pos] == '\\') ++pos;
                ++pos;
            }
        }
        ++pos;
    }
    return j.substr(start, pos - start);
}

std::string escapeString(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 4);
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) { char b[8]; std::snprintf(b,sizeof(b),"\\u%04x",c); out+=b; }
                else out += (char)c;
        }
    }
    return out;
}

std::string makeString(const std::string& key, const std::string& val) {
    return "\"" + escapeString(key) + "\":\"" + escapeString(val) + "\"";
}

std::string makeNumber(const std::string& key, double val) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6);
    ss << "\"" << escapeString(key) << "\":" << val;
    return ss.str();
}

std::string makeNumber(const std::string& key, long long val) {
    return "\"" + escapeString(key) + "\":" + std::to_string(val);
}

std::string makeBool(const std::string& key, bool val) {
    return "\"" + escapeString(key) + "\":" + (val ? "true" : "false");
}

std::string makeObject(const std::vector<std::string>& fields) {
    std::string out = "{";
    for (size_t i = 0; i < fields.size(); ++i) { if (i) out += ","; out += fields[i]; }
    return out + "}";
}

std::string makeArray(const std::vector<std::string>& items) {
    std::string out = "[";
    for (size_t i = 0; i < items.size(); ++i) { if (i) out += ","; out += items[i]; }
    return out + "]";
}

}
