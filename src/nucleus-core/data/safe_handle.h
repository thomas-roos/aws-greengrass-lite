#pragma once
#include <memory>

namespace data {
    class ObjectAnchor;

    //
    // Base class for handles
    //
    class Handle {
    private:
        uint32_t _asInt;
    public:
        struct CompEq {
            [[nodiscard]] bool operator()(const Handle a, const Handle b) const noexcept {
                return a._asInt == b._asInt;
            }
        };
        struct CompLess {
            [[nodiscard]] bool operator()(const Handle a, const Handle b) const noexcept {
                return a._asInt < b._asInt;
            }
        };
        struct Hash {
            [[nodiscard]] std::size_t operator()(const Handle k) const noexcept {
                return k._asInt;
            }
        };

        constexpr Handle() : _asInt{0} {
        }
        explicit constexpr Handle(const uint32_t i) : _asInt{i} {
        }
        constexpr Handle(const Handle &) = default;
        constexpr Handle(Handle &&) = default;
        ~Handle() = default;
        explicit Handle(const std::shared_ptr<ObjectAnchor> & anchored);
        explicit Handle(ObjectAnchor * anchored);

        constexpr Handle & operator=(const Handle &) = default;
        constexpr Handle & operator=(Handle &&) = default;

        [[nodiscard]] uint32_t asInt() const {
            return _asInt;
        }

        [[nodiscard]] bool isNull() const {
            return _asInt == 0;
        }

        explicit operator bool() const {
            return _asInt != 0;
        }

        bool operator !() const {
            return _asInt == 0;
        }

        bool operator==(Handle other) const {
            return _asInt == other._asInt;
        }

        bool operator!=(Handle other) const {
            return _asInt != other._asInt;
        }

        static constexpr Handle nullHandle();
    };

    inline constexpr Handle Handle::nullHandle() {
        return {};
    }
}
