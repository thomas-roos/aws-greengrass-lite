#include "handle_table.h"
#include "environment.h"

Handle::Handle(const std::shared_ptr<Anchored> & anchored) {
    if (!anchored) {
        _asInt = 0;
    } else {
        _asInt = anchored->getHandle()._asInt;
    }
}

Handle::Handle(Anchored * anchored) {
    if (!anchored) {
        _asInt = 0;
    } else {
        _asInt = anchored->getHandle()._asInt;
    }
}

Handle Anchored::getHandle() {
    return _environment.handleTable.getHandle(this);
}

void HandleTable::release(Anchored * anchored) {
    if (!anchored) {
        return;
    }
    Handle h = getExistingHandle(anchored);
    if (h) {
        std::unique_lock guard{_mutex};
        _handles.erase(h);
        anchored->_handle = Handle::nullHandle;
    }
}

Handle HandleTable::getHandle(Anchored * anchored) {
    if (!anchored) {
        return Handle::nullHandle;
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

Anchored::~Anchored() {
    _environment.handleTable.release(this);
}

std::shared_ptr<Anchored> AnchoredWithRoots::anchor(AnchoredObject *obj) {
    if (!obj) {
        return nullptr;
    }
    auto ptr { std::make_shared<Anchored>(_environment, obj)};
    Handle h = ptr->getHandle();
    std::unique_lock guard{_mutex};
    _roots[h] = ptr;
    auto self = std::dynamic_pointer_cast<AnchoredWithRoots>(shared_from_this());
    ptr->_owner = self;
    return ptr;
}

std::shared_ptr<Anchored> AnchoredWithRoots::anchor(const std::shared_ptr<Anchored>& anchored) {
    if (!anchored) {
        return nullptr;
    }
    return anchor(anchored.get());
}

std::shared_ptr<Anchored> AnchoredWithRoots::anchor(Anchored *anchored) {
    if (!anchored) {
        return nullptr;
    }
    return anchor(anchored->getObject<AnchoredObject>().get());
}

std::shared_ptr<Anchored> AnchoredWithRoots::anchor(Handle handle) {
    if (!handle) {
        return nullptr;
    }
    return anchor(_environment.handleTable.getObject<AnchoredObject>(handle).get());
}

bool AnchoredWithRoots::release(Handle handle) {
    if (!handle) {
        return false;
    }
    std::unique_lock guard{_mutex};
    return _roots.erase(handle) > 0;
}

bool Anchored::release() {
    std::shared_ptr<AnchoredWithRoots> owner {_owner};
    return owner->release(this);
}

bool AnchoredWithRoots::release(Anchored * anchored) {
    if (!anchored) {
        return false;
    }
    return release(anchored->getHandle());
}

bool AnchoredWithRoots::release(std::shared_ptr<Anchored>& anchored) {
    if (!anchored) {
        return false;
    }
    return release(anchored->getHandle());
}
