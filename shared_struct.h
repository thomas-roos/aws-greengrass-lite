#pragma once
#include "environment.h"
#include "handle_table.h"
#include <vector>
#include <mutex>
#include "environment.h"
#include "handle_table.h"
#include <vector>
#include <mutex>
#include <map>
#include <set>
#include <variant>

class SharedStruct;

class StructElement {
private:
    // size of variant is prob ~ 32 bytes or so, because of std::string
    // in theory, can be optimized, but is it worth it?
    std::variant<std::monostate, uint64_t, double, std::shared_ptr<SharedStruct>, std::string> _value;
    static constexpr int VOID = 0;
    static constexpr int INT = 1;
    static constexpr int DOUBLE = 2;
    static constexpr int STRUCT = 3;
    static constexpr int STRING = 4;

    void rootsCheck(const SharedStruct * target) const;

public:
    friend class SharedStruct;
    static const StructElement nullElement;
    StructElement() : _value{} {
    }
    explicit StructElement(const uint64_t v) : _value{v} {
    }
    explicit StructElement(const double v) : _value{v} {
    }
    explicit StructElement(SharedStruct * p);
    explicit StructElement(std::shared_ptr<SharedStruct> p);
    explicit StructElement(const std::string & str) : _value{str} {
    }
    StructElement(const StructElement & other) = default;
    StructElement & operator=(const StructElement & other) = default;
    explicit operator bool() const {
        return _value.index() != VOID;
    }
    bool operator!() const {
        return _value.index() == VOID;
    }
    [[nodiscard]] bool isStruct() const {
        return _value.index() == STRUCT;
    }
    [[nodiscard]] uint64_t getInt() const {
        switch (_value.index()) {
            case INT:
                return std::get<uint64_t>(_value);
            case DOUBLE:
                return static_cast<uint64_t>(std::get<double>(_value));
            case STRING:
                return std::stoul(std::get<std::string>(_value));
            default:
                throw std::runtime_error("Unsupported type conversion to integer");
        }
    }
    [[nodiscard]] double getDouble() const {
        switch (_value.index()) {
            case INT:
                return static_cast<double>(std::get<uint64_t>(_value));
            case DOUBLE:
                return std::get<double>(_value);
            case STRING:
                return std::stod(std::get<std::string>(_value));
            default:
                throw std::runtime_error("Unsupported type conversion to double");
        }
    }
    [[nodiscard]] std::string getString() const {
        switch (_value.index()) {
            case INT:
                return std::to_string(std::get<uint64_t>(_value));
            case DOUBLE:
                return std::to_string(std::get<double>(_value));
            case STRING:
                return std::get<std::string>(_value);
            default:
                throw std::runtime_error("Unsupported type conversion to string");
        }
    }
    [[nodiscard]] std::shared_ptr<SharedStruct> getStructRef() {
        switch (_value.index()) {
            case STRUCT:
                return std::get<std::shared_ptr<SharedStruct>>(_value);
            default:
                throw std::runtime_error("Unsupported type conversion to object");
        }
    }

    explicit operator uint64_t() const {
        return getInt();
    }
    explicit operator uint32_t() const {
        return static_cast<uint32_t>(getInt());
    }
    explicit operator double() const {
        return getDouble();
    }
    explicit operator float() const {
        return static_cast<float>(getDouble());
    }
    explicit operator std::string() const {
        return getString();
    }
};

inline const StructElement StructElement::nullElement {};

class SharedStruct : public AnchoredObject {
private:
    std::map<Handle, StructElement, Handle::CompLess> _elements;

    void rootsCheck(const StructElement & element) const {
        element.rootsCheck(this);
    }

    void rootsCheck(const SharedStruct * target) const;

public:
    friend class StructElement;
    explicit SharedStruct(Environment & environment) : AnchoredObject{environment} {
    }
    std::shared_ptr<SharedStruct> struct_shared_from_this() {
        return std::dynamic_pointer_cast<SharedStruct>(shared_from_this());
    }

    void put(const Handle handle, const StructElement & element) {
        _environment.stringTable.assertStringHandle(handle);
        std::unique_lock guard {_environment.sharedStructMutex};
        rootsCheck(element); // in lock
        _elements[handle] = element;
    }

    void put(const std::string_view sv, const StructElement & element) {
        Handle handle = _environment.stringTable.getOrCreateOrd(std::string(sv));
        std::unique_lock guard {_environment.sharedStructMutex};
        rootsCheck(element); // in lock
        _elements[handle] = element;
    }

    bool hasKey(const Handle handle) {
        //_environment.stringTable.assertStringHandle(handle);
        std::shared_lock guard {_environment.sharedStructMutex};
        auto i = _elements.find(handle);
        return i != _elements.end();
    }

    StructElement get(const Handle handle) const {
        //_environment.stringTable.assertStringHandle(handle);
        std::shared_lock guard {_environment.sharedStructMutex};
        auto i = _elements.find(handle);
        if (i == _elements.end()) {
            return StructElement::nullElement;
        } else {
            return i->second;
        }
    }

    StructElement get(const std::string_view sv) const {
        Handle handle = _environment.stringTable.getOrCreateOrd(std::string(sv));
        return get(handle);
    }
};

inline StructElement::StructElement(SharedStruct * p) : _value{p->struct_shared_from_this()} {
}
inline StructElement::StructElement(std::shared_ptr<SharedStruct> p) : _value{p} {
}
