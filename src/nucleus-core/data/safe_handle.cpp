#include "safe_handle.h"
#include "tracked_object.h"

data::Handle::Handle(const std::shared_ptr<data::ObjectAnchor> & anchored) {
    if (!anchored) {
        _asInt = 0;
    } else {
        _asInt = anchored->getHandle()._asInt;
    }
}

data::Handle::Handle(data::ObjectAnchor * anchored) {
    if (!anchored) {
        _asInt = 0;
    } else {
        _asInt = anchored->getHandle()._asInt;
    }
}
