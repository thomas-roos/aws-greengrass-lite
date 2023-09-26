#include "shared_struct.h"
#include "environment.h"
#include "safe_handle.h"

bool data::SharedStruct::putStruct(const data::Handle handle, const data::StructElement &element) {
    if (!element.isStruct()) {
        return false; // not a structure
    }
    std::shared_ptr<data::Structish> otherStruct = element.getStructRef();
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

void data::SharedStruct::rootsCheck(const data::Structish *target) const { // NOLINT(*-no-recursion)
    if (this == target) {
        throw std::runtime_error("Recursive reference of structure");
    }
    // we don't want to keep nesting locks else we will deadlock
    std::shared_lock guard {_mutex };
    std::vector<std::shared_ptr<data::Structish>> structs;
    for (auto const & i : _elements) {
        if (i.second.isStruct()) {
            std::shared_ptr<data::Structish> otherStruct = i.second.getStructRef();
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

std::shared_ptr<data::Structish> data::SharedStruct::copy() const {
    std::shared_ptr<data::Structish> newCopy {std::make_shared<data::SharedStruct>(_environment)};
    std::shared_lock guard {_mutex}; // for source
    for (auto const & i : _elements) {
        newCopy->put(i.first, i.second);
    }
    return newCopy;
}

void data::SharedStruct::put(const data::Handle handle, const data::StructElement & element) {
    _environment.stringTable.assertStringHandle(handle);
    if (!(element.isStruct() && putStruct(handle, element))) {
        std::unique_lock guard{_mutex};
        _elements[handle] = element;
    }
}

void data::SharedStruct::put(const std::string_view sv, const data::StructElement & element) {
    data::Handle handle = _environment.stringTable.getOrCreateOrd(std::string(sv));
    put(handle, element);
}

bool data::SharedStruct::hasKey(const data::Handle handle) {
    //_environment.stringTable.assertStringHandle(handle);
    std::shared_lock guard {_mutex};
    auto i = _elements.find(handle);
    return i != _elements.end();
}

data::StructElement data::SharedStruct::get(const std::string_view sv) const {
    data::Handle handle = _environment.stringTable.getOrCreateOrd(std::string(sv));
    return get(handle);
}

data::StructElement data::SharedStruct::get(data::Handle handle) const {
    //_environment.stringTable.assertStringHandle(handle);
    std::shared_lock guard {_mutex};
    auto i = _elements.find(handle);
    if (i == _elements.end()) {
        return data::StructElement::nullElement;
    } else {
        return i->second;
    }
}
