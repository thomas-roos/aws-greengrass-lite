#include "shared_struct.h"
#include "environment.h"

bool SharedStruct::putStruct(const Handle handle, const StructElement &element) {
    if (!element.isStruct()) {
        return false; // not a structure
    }
    std::shared_ptr<Structish> otherStruct = element.getStructRef();
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

void SharedStruct::rootsCheck(const Structish *target) const { // NOLINT(*-no-recursion)
    if (this == target) {
        throw std::runtime_error("Recursive reference of structure");
    }
    // we don't want to keep nesting locks else we will deadlock
    std::shared_lock guard {_mutex };
    std::vector<std::shared_ptr<Structish>> structs;
    for (auto const & i : _elements) {
        if (i.second.isStruct()) {
            std::shared_ptr<Structish> otherStruct = i.second.getStructRef();
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

std::shared_ptr<Structish> SharedStruct::copy() const {
    std::shared_ptr<Structish> newCopy {std::make_shared<SharedStruct>(_environment)};
    std::shared_lock guard {_mutex}; // for source
    for (auto const & i : _elements) {
        newCopy->put(i.first, i.second);
    }
    return newCopy;
}

void SharedStruct::put(const Handle handle, const StructElement & element) {
    _environment.stringTable.assertStringHandle(handle);
    if (!(element.isStruct() && putStruct(handle, element))) {
        std::unique_lock guard{_mutex};
        _elements[handle] = element;
    }
}

void SharedStruct::put(const std::string_view sv, const StructElement & element) {
    Handle handle = _environment.stringTable.getOrCreateOrd(std::string(sv));
    put(handle, element);
}

bool SharedStruct::hasKey(const Handle handle) {
    //_environment.stringTable.assertStringHandle(handle);
    std::shared_lock guard {_mutex};
    auto i = _elements.find(handle);
    return i != _elements.end();
}

StructElement SharedStruct::get(const std::string_view sv) const {
    Handle handle = _environment.stringTable.getOrCreateOrd(std::string(sv));
    return get(handle);
}

StructElement SharedStruct::get(Handle handle) const {
    //_environment.stringTable.assertStringHandle(handle);
    std::shared_lock guard {_mutex};
    auto i = _elements.find(handle);
    if (i == _elements.end()) {
        return StructElement::nullElement;
    } else {
        return i->second;
    }
}
