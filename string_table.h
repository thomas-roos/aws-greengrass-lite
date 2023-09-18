#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <shared_mutex>
#include <mutex>
#include <string>
#include <string_view>
#include "safe_handle.h"

class StringTable {
private:
    mutable std::shared_mutex _mutex;
    std::unordered_map<std::string, Handle> _interned;
    std::unordered_map<Handle, std::string, Handle::Hash, Handle::CompEq> _reverse;
public:
    Handle testAndGetOrd(std::string_view str_ref) const {
        std::string copy {str_ref}; // desired feature in C++20 not C++17
        return testAndGetOrd(copy);
    }

    Handle testAndGetOrd(std::string const & str) const {
        std::shared_lock guard {_mutex};
        auto i = _interned.find(str);
        if (i == _interned.end()) {
            return Handle::nullHandle;
        } else {
            return i->second;
        }
    }

    Handle getOrCreateOrd(std::string_view str_ref) {
        std::string copy{str_ref}; // desired feature in C++20 not C++17
        return getOrCreateOrd(copy);
    }

    Handle getOrCreateOrd(std::string const & str) {
        Handle ord = testAndGetOrd(str); // optimistic using shared lock
        if (ord.isNull()) {
            std::unique_lock guard(_mutex);
            auto i = _interned.find(str);
            if (i == _interned.end()) {
                // expected case
                // ordinals are computed this way to optimize std::map
                Handle new_ord {static_cast<uint32_t >(std::hash<std::string>()(str))}; // distribute IDs
                while (_reverse.find(new_ord) != _reverse.end()) {
                    new_ord = Handle {new_ord.asInt() + 15299}; // prime number
                }
                _reverse[new_ord] = str;
                _interned[str] = new_ord;
                return new_ord;
            } else {
                // this path handles a race condition
                return i->second;
            }
        } else {
            return ord;
        }
    }

    bool isStringOrdValid(Handle ord) const {
        std::shared_lock guard {_mutex};
        return _reverse.find(ord) != _reverse.end();
    }

    std::string getString(Handle ord) const {
        std::shared_lock guard {_mutex};
        return _reverse.at(ord);
    }

    void assertStringHandle(const Handle ord) const {
        if (!isStringOrdValid(ord)) {
            throw std::invalid_argument("String ordinal is not valid");
        }
    }
};

class CheckedBuffer {
private:
    char * _buffer;
    size_t _buflen;
public:
    explicit CheckedBuffer(char * buffer, size_t bufLen) :
        _buffer{buffer},_buflen(_buflen) {
        if (_buffer + _buflen < _buffer) {
            throw std::out_of_range("Buffer wraps");
        }
    }

    size_t copy(const std::string & s) {
        if (_buflen < 1 || s.length() >= _buflen) {
            throw std::out_of_range("Buffer is too small");
        }
        s.copy(_buffer, _buflen-1);
        _buffer[s.length()] = 0; // ok because of above length check
        return s.length();
    }
};