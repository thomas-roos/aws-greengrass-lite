#include "handle_table.h"
#include "environment.h"

void data::HandleTable::release(ObjectAnchor * anchored) {
    if (!anchored) {
        return;
    }
    Handle h = getExistingHandle(anchored);
    if (h) {
        std::unique_lock guard{_mutex};
        _handles.erase(h);
        anchored->_handle = Handle::nullHandle();
    }
}

data::Handle data::HandleTable::getHandle(ObjectAnchor * anchored) {
    if (!anchored) {
        return Handle::nullHandle();
    }
    Handle h = getExistingHandle(anchored);
    if (!h) {
        std::unique_lock guard {_mutex};
        Handle cached = anchored->_handle;
        if (!cached) {
            Handle new_handle {++_counter};
            for (;;) {
                // typically once, but will keep repeating if there is a handle collision
                // TODO: consider using RNG to provide initial handle value
                auto i = _handles.find(new_handle);
                if (i == _handles.end()) {
                    anchored->_handle = new_handle;
                    _handles[new_handle] = anchored->weak_from_this();
                    return new_handle;
                }
                // here if handles wrapped
                // prime number - assumes handles cluster
                // we also assume we will eventually get a handle
                new_handle = Handle{new_handle.asInt() + 1033};
            }
        } else {
            return cached;
        }
    } else {
        return h;
    }
}

data::ObjectAnchor::~ObjectAnchor() {
    _environment.handleTable.release(this);
}

std::shared_ptr<data::ObjectAnchor> data::TrackingScope::anchor(TrackedObject *obj) {
    if (!obj) {
        return nullptr;
    }
    auto ptr { std::make_shared<ObjectAnchor>(_environment, obj)};
    Handle h = ptr->getHandle();
    std::unique_lock guard{_mutex};
    _roots[h] = ptr;
    auto self = std::dynamic_pointer_cast<TrackingScope>(shared_from_this());
    ptr->_owner = self;
    return ptr;
}

std::shared_ptr<data::ObjectAnchor> data::TrackingScope::anchor(const std::shared_ptr<ObjectAnchor>& anchored) {
    if (!anchored) {
        return nullptr;
    }
    return anchor(anchored.get());
}

std::shared_ptr<data::ObjectAnchor> data::TrackingScope::anchor(ObjectAnchor *anchored) {
    if (!anchored) {
        return nullptr;
    }
    return anchor(anchored->getObject<TrackedObject>().get());
}

std::shared_ptr<data::ObjectAnchor> data::TrackingScope::anchor(Handle handle) {
    if (!handle) {
        return nullptr;
    }
    return anchor(_environment.handleTable.getObject<TrackedObject>(handle).get());
}

bool data::TrackingScope::release(Handle handle) {
    if (!handle) {
        return false;
    }
    std::unique_lock guard{_mutex};
    return _roots.erase(handle) > 0;
}

bool data::ObjectAnchor::release() {
    std::shared_ptr<TrackingScope> owner {_owner};
    return owner->release(this);
}

bool data::TrackingScope::release(ObjectAnchor * anchored) {
    if (!anchored) {
        return false;
    }
    return release(anchored->getHandle());
}

bool data::TrackingScope::release(std::shared_ptr<ObjectAnchor>& anchored) {
    if (!anchored) {
        return false;
    }
    return release(anchored->getHandle());
}
