#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace miqu {

class StringUtils {
public:
    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    static std::string trim(const std::string& str, const std::string& chars) {
        size_t first = str.find_first_not_of(chars);
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(chars);
        return str.substr(first, (last - first + 1));
    }

    static std::string to_lower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
        return s;
    }

    static std::string to_upper(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
            return std::toupper(c);
        });
        return s;
    }

    static bool starts_with(const std::string& str, const std::string& prefix) {
        if (prefix.size() > str.size()) return false;
        return str.compare(0, prefix.size(), prefix) == 0;
    }

    static bool ends_with(const std::string& str, const std::string& suffix) {
        if (suffix.size() > str.size()) return false;
        return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    static std::vector<std::string> split(const std::string& str, char delim) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;
        while (std::getline(ss, token, delim)) {
            tokens.push_back(token);
        }
        return tokens;
    }
};

} // namespace miqu
