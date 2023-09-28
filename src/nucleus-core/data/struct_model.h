#pragma once
#include "string_table.h"
#include "handle_table.h"
#include <vector>
#include <mutex>
#include <map>
#include <set>
#include <variant>

namespace data {
    class StructModelBase;

    typedef std::variant<std::monostate, uint64_t, double, std::shared_ptr<StructModelBase>, std::string> ValueType;

    //
    // Data storage element with implicit type conversion
    //
    class StructElement {
        friend class StructModelBase;
    protected:
        // size of variant is prob ~ 32 bytes or so, because of std::string
        // in theory, can be optimized, but is it worth it?
        ValueType _value;
        static constexpr int NONE = 0;
        static constexpr int INT = 1;
        static constexpr int DOUBLE = 2;
        static constexpr int STRING = 3;
        static constexpr int STRUCT = 4;
        static constexpr int ARRAY = 5;

    public:
        StructElement() : _value{} {
        }
        explicit StructElement(ValueType v) : _value(std::move(v)) {
        }
        explicit StructElement(const uint64_t v) : _value{v} {
        }
        explicit StructElement(const double v) : _value{v} {
        }
        explicit StructElement(std::shared_ptr<StructModelBase> p);
        explicit StructElement(const std::string & str) : _value{str} {
        }
        StructElement(const StructElement &) = default;
        StructElement(StructElement &&) = default;
        StructElement & operator=(const StructElement &) = default;
        StructElement & operator=(StructElement &&) = default;
        ~StructElement() = default;

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
        [[nodiscard]] std::shared_ptr<StructModelBase> getStructRef() const {
            switch (_value.index()) {
                case STRUCT:
                    return std::get<std::shared_ptr<StructModelBase>>(_value);
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

    //
    // Base class for classes that behave like a structure - common between shared structures and config
    //
    class StructModelBase : public TrackedObject {
    public:
        friend class StructElement;
        explicit StructModelBase(Environment & environment) : TrackedObject{environment} {
        }

        virtual void rootsCheck(const StructModelBase * target) const = 0;
        virtual void put(StringOrd handle, const StructElement & element) = 0;
        virtual void put(std::string_view sv, const StructElement & element) = 0;
        virtual bool hasKey(StringOrd handle)  = 0;
        virtual StructElement get(StringOrd handle) const = 0;
        virtual StructElement get(std::string_view sv) const = 0;
        virtual std::shared_ptr<StructModelBase> copy() const = 0;
    };

    inline StructElement::StructElement(std::shared_ptr<StructModelBase> p) : _value{p} {
    }

}