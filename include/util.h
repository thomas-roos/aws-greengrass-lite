#pragma once
#include <memory>
#include <string>
#include <functional>

namespace util {
    inline bool startsWith(std::string_view target, std::string_view prefix) {
        // prefix that target string starts with prefix string
        if (prefix.length() > target.length()) {
            return false;
        }
        return target.substr(0, prefix.length()) == prefix;
    }

    inline bool endsWith(std::string_view target, std::string_view suffix) {
        // prefix that target string starts with prefix string
        if (suffix.length() > target.length()) {
            return false;
        }
        return target.substr(target.length()-suffix.length(), suffix.length()) == suffix;
    }

    inline std::string_view trimStart(std::string_view target, std::string_view prefix) {
        // remove prefix from start
        if (startsWith(target, prefix)) {
            return target.substr(prefix.length(), target.length()-prefix.length());
        } else {
            return target;
        }
    }

    inline std::string_view trimEnd(std::string_view target, std::string_view suffix) {
        // remove suffix from end
        if (endsWith(target, suffix)) {
            return target.substr(0, target.length()-suffix.length());
        } else {
            return target;
        }
    }

    inline int lowerChar(int c) {
        // important: ignore Locale to ensure portability
        if (c >= 'A' && c <= 'Z') {
            return c-'A'+'a';
        } else {
            return c;
        }
    }

    inline std::string lower(std::string_view source) {
        std::string target;
        std::transform(source.begin(), source.end(), target.begin(), lowerChar);
        return target;
    }
}
