#include "environment.h"

ExpireTime Environment::translateExpires(int32_t delta) {
    // override this to enable time-based testing
    return ExpireTime::fromNow(delta);
}

namespace data {
    void SysProperties::parseEnv(char * envp[]) {
        std::unique_lock guard {_mutex};
        char ** p;
        for (p = envp; *p != nullptr; ++p) {
            char * key = *p;
            char * eq;
            for (eq = key; *eq != 0; ++eq) {
                if (*eq == '=') {
                    break;
                }
            }
            if (*eq) {
                _cache.emplace(std::string(key, eq-key), std::string(eq+1));
            } else {
                _cache.emplace(std::string(key, eq-key), "");
            }
        }
    }

    std::optional<std::string> SysProperties::get(std::string_view name) const {
        std::shared_lock guard {_mutex};
        std::string key {name};
        const auto &i = _cache.find(key);
        if (i == _cache.end()) {
            return {};
        } else {
            return i->second;
        }
    }

    bool SysProperties::exists(std::string_view name) const {
        std::shared_lock guard {_mutex};
        std::string key {name};
        const auto &i = _cache.find(key);
        return i != _cache.end();
    }

    void SysProperties::put(std::string_view name, const std::string & value) {
        std::unique_lock guard {_mutex};
        _cache.emplace(name, value);
    }

    void SysProperties::remove(std::string_view name) {
        std::unique_lock guard {_mutex};
        std::string key {name};
        _cache.erase(key);
    }
}
