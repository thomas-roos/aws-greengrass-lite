#pragma once

#include "shared_struct.h"
#include "handle_table.h"
#include "string_table.h"
#include "expire_time.h"

namespace config {

    //
    // Greengrass-Java use of config timestamp can be considered to be a signed long representing seconds since
    // epoch. Given the special constants, it's better to handle as 32-bit signed integer rather then handle all
    // the weird edge conditions.
    //
    class Timestamp {
    private:
        int32_t _time; // since epoch
    public:
        constexpr Timestamp() : _time{0} {
        }

        constexpr Timestamp(int32_t timeSecs) : _time{timeSecs} {
        }

        constexpr Timestamp(const Timestamp &time) : _time{time._time} {
        }

        constexpr explicit Timestamp(const std::chrono::time_point<std::chrono::system_clock> time) :
                _time(static_cast<int32_t>(std::chrono::duration_cast<std::chrono::seconds>(time.time_since_epoch()).count())) {
        }

//        static Timestamp now() {
//            return Timestamp{std::chrono::system_clock::now()};
//        }
//
        [[nodiscard]] constexpr int32_t asSeconds() const {
            return _time;
        };

        constexpr bool operator==(const Timestamp &other) const {
            return _time == other._time;
        }

        constexpr Timestamp &operator=(const Timestamp &time) {
            _time = time._time;
            return *this;
        }

        static constexpr Timestamp never();
        static constexpr Timestamp dawn();
        static constexpr Timestamp infinite();
    };

    constexpr Timestamp Timestamp::never() {
        return {0};
    }
    constexpr Timestamp Timestamp::dawn() {
        return {1};
    }
    constexpr Timestamp Timestamp::infinite() {
        return {-1};
    }

    class Manager {

    };
}
