#include "tracked_object.h"
#include "environment.h"

data::Handle data::ObjectAnchor::getHandle() {
    return _environment.handleTable.getHandle(this);
}
