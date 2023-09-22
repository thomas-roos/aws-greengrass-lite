#include <unordered_set>
#include <cassert>
#include "shared_struct.h"

bool SharedStruct::putStruct(const Handle handle, const StructElement &element) {
    if (!element.isStruct()) {
        return false; // not a structure
    }
    std::shared_ptr<SharedStruct> otherStruct = element.getStructRef();
    if (!otherStruct) {
        return false; // structure is null handle
    }
    // prepare for cycle checks
    // cycle checking requires obtaining the cycle check mutex
    // the structure mutex must be acquired after cycle check mutex
    // TODO: there has to be a better way
    std::unique_lock cycleGuard {_environment.cycleCheckMutex};
    otherStruct->rootsCheck(this);
    _elements[handle] = element;

    return true;
}

void SharedStruct::rootsCheck(const SharedStruct *target) const { // NOLINT(*-no-recursion)
    if (this == target) {
        throw std::runtime_error("Recursive reference of structure");
    }
    // we don't want to keep nesting locks else we will deadlock
    std::shared_lock guard {_mutex };
    std::vector<std::shared_ptr<SharedStruct>> structs;
    for (auto const & i : _elements) {
        if (i.second.isStruct()) {
            std::shared_ptr<SharedStruct> otherStruct = i.second.getStructRef();
            if (otherStruct) {
                structs.emplace_back(otherStruct);
            }
        }
    }
    guard.release();
    for (auto const & i : structs) {
        i->rootsCheck(target);
    }
}

std::shared_ptr<SharedStruct> SharedStruct::copy() const {
    std::shared_ptr<SharedStruct> newCopy {std::make_shared<SharedStruct>(_environment)};
    std::shared_lock guard {_mutex}; // for source
    for (auto const & i : _elements) {
        newCopy->put(i.first, i.second);
    }
    return newCopy;
}
