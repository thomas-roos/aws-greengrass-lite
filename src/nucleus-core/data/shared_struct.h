#pragma once
#include "handle_table.h"
#include <vector>
#include <mutex>
#include <map>
#include <set>
#include <variant>

class Structish;
class Environment;

class StructElement {
public:
    typedef std::variant<std::monostate, uint64_t, double, std::shared_ptr<Structish>, std::string> ValueType;
protected:
    // size of variant is prob ~ 32 bytes or so, because of std::string
    // in theory, can be optimized, but is it worth it?
    ValueType _value;
    static constexpr int NONE = 0;
    static constexpr int INT = 1;
    static constexpr int DOUBLE = 2;
    static constexpr int STRUCT = 3;
    static constexpr int STRING = 4;

public:
    friend class Structish;
    static const StructElement nullElement;
    StructElement() : _value{} {
    }
    explicit StructElement(ValueType v) : _value(std::move(v)) {
    }
    explicit StructElement(const uint64_t v) : _value{v} {
    }
    explicit StructElement(const double v) : _value{v} {
    }
    explicit StructElement(Structish * p);
    explicit StructElement(std::shared_ptr<Structish> p);
    explicit StructElement(const std::string & str) : _value{str} {
    }
    StructElement(const StructElement & other) = default;
    StructElement & operator=(const StructElement & other) = default;
    explicit operator bool() const {
        return _value.index() != NONE;
    }
    bool operator!() const {
        return _value.index() == NONE;
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
    [[nodiscard]] std::shared_ptr<Structish> getStructRef() const {
        switch (_value.index()) {
            case STRUCT:
                return std::get<std::shared_ptr<Structish>>(_value);
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

//
// Base class for classes that behave like a structure - relevant for config
//
class Structish : public AnchoredObject {
public:
    friend class StructElement;
    explicit Structish(Environment & environment) : AnchoredObject{environment} {
    }
    std::shared_ptr<Structish> structish_shared_from_this() {
        return std::dynamic_pointer_cast<Structish>(shared_from_this());
    }

    virtual void rootsCheck(const Structish * target) const = 0;
    virtual void put(Handle handle, const StructElement & element) = 0;
    virtual void put(std::string_view sv, const StructElement & element) = 0;
    virtual bool hasKey(Handle handle)  = 0;
    virtual StructElement get(Handle handle) const = 0;
    virtual StructElement get(std::string_view sv) const = 0;
    virtual std::shared_ptr<Structish> copy() const = 0;
};

inline StructElement::StructElement(Structish * p) : _value{p->structish_shared_from_this()} {
}
inline StructElement::StructElement(std::shared_ptr<Structish> p) : _value{p} {
}

/**
 * Typical implementation of Structish
 */
class SharedStruct : public Structish {
protected:
    std::map<Handle, StructElement, Handle::CompLess> _elements;
    mutable std::shared_mutex _mutex;

    bool putStruct(Handle handle, const StructElement & element);

    void rootsCheck(const Structish * target) const override;

public:
    explicit SharedStruct(Environment & environment) : Structish{environment} {
    }
    std::shared_ptr<SharedStruct> struct_shared_from_this() {
        return std::dynamic_pointer_cast<SharedStruct>(shared_from_this());
    }

    void put(Handle handle, const StructElement & element) override;
    void put(std::string_view sv, const StructElement & element) override;
    bool hasKey(Handle handle) override;
    StructElement get(Handle handle) const override;
    StructElement get(std::string_view sv) const override;
    std::shared_ptr<Structish> copy() const override;
};
