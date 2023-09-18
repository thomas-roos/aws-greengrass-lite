#include "environment.h"

time_t Environment::relativeToAbsoluteTime(time_t relTime) {
    // override this to enable time-based testing
    time_t now { std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) };
    time_t newTime = relTime + now;
    if (relTime < 0 && newTime > 0) {
        newTime = -1; // don't wrap
    }
    return newTime;
}
