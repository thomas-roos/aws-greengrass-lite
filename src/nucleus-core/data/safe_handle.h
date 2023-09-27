#pragma once
#include <memory>

namespace data {
    class Anchored;

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

        Handle() : _asInt{0} {
        }
        explicit Handle(const uint32_t i) : _asInt{i} {
        }
        Handle(const Handle &) = default;
        Handle(Handle &&) = default;
        ~Handle() = default;
        explicit Handle(const std::shared_ptr<Anchored> & anchored);
        explicit Handle(Anchored * anchored);

        Handle & operator=(const Handle &) = default;
        Handle & operator=(Handle &&) = default;

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

        static const Handle nullHandle;
    };

    inline const Handle Handle::nullHandle{};
}
