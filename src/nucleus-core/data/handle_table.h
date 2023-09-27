#pragma once
#include "safe_handle.h"
#include "tracked_object.h"
#include <util.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <map>
#include <shared_mutex>
#include <mutex>
#include <memory>

namespace data {
    //
    // Handle tracking
    //
    class HandleTable {
    private:
        mutable std::shared_mutex _mutex; // shared_mutex would be better
        uint32_t _counter {0};
        std::unordered_map<Handle, std::weak_ptr<ObjectAnchor>, Handle::Hash, Handle::CompEq> _handles;

    public:
        Handle getExistingHandle(const ObjectAnchor * anchored) const {
            std::shared_lock guard {_mutex};
            return anchored->_handle;
        }

        Handle getHandle(ObjectAnchor * anchored);
        void release(ObjectAnchor * anchored);

        std::shared_ptr<ObjectAnchor> getAnchor(const Handle handle) {
            std::shared_lock guard {_mutex};
            auto i = _handles.find(handle);
            if (i == _handles.end()) {
                return nullptr;
            }
            return i->second.lock();
        }

        template<typename T>
        std::shared_ptr<T> getObject(const Handle handle) {
            std::shared_ptr<ObjectAnchor> obj {getAnchor(handle)};
            if (obj) {
                return obj->getObject<T>();
            } else {
                return nullptr;
            }
        }

    };
}

