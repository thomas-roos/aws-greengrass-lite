#include "environment.h"

ExpireTime Environment::translateExpires(int32_t delta) {
    // override this to enable time-based testing
    return ExpireTime::fromNow(delta);
}
