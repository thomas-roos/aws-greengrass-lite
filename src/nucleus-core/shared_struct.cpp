#include "shared_struct.h"

void StructElement::rootsCheck(const SharedStruct *target) const { // NOLINT(*-no-recursion)
    if (!isStruct()) {
        return;
    }
    std::get<std::shared_ptr<SharedStruct>>(_value)->rootsCheck(target);
}

void SharedStruct::rootsCheck(const SharedStruct *target) const { // NOLINT(*-no-recursion)
    if (this == target) {
        throw std::runtime_error("Recursive reference of structure");
    }
    // assume lock acquired
    for (auto const & i : _elements) {
        if (i.second.isStruct()) {
            i.second.rootsCheck(target);
        }
    }
}
