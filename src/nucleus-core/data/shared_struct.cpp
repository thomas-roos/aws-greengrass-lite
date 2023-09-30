#include "shared_struct.h"
#include "environment.h"
#include "safe_handle.h"

void data::SharedStruct::rootsCheck(const data::ContainerModelBase *target) const { // NOLINT(*-no-recursion)
    if (this == target) {
        throw std::runtime_error("Recursive reference of container");
    }
    // we don't want to keep nesting locks else we will deadlock
    std::shared_lock guard {_mutex };
    std::vector<std::shared_ptr<data::ContainerModelBase>> containers;
    for (auto const & i : _elements) {
        if (i.second.isContainer()) {
            std::shared_ptr<data::ContainerModelBase> otherContainer = i.second.getContainer();
            if (otherContainer) {
                containers.emplace_back(otherContainer);
            }
        }
    }
    guard.release();
    for (auto const & i : containers) {
        i->rootsCheck(target);
    }
}

void data::SharedList::rootsCheck(const data::ContainerModelBase *target) const { // NOLINT(*-no-recursion)
    if (this == target) {
        throw std::runtime_error("Recursive reference of container");
    }
    // we don't want to keep nesting locks else we will deadlock
    std::shared_lock guard {_mutex };
    std::vector<std::shared_ptr<data::ContainerModelBase>> containers;
    for (auto const & i : _elements) {
        if (i.isContainer()) {
            std::shared_ptr<data::ContainerModelBase> otherContainer = i.getContainer();
            if (otherContainer) {
                containers.emplace_back(otherContainer);
            }
        }
    }
    guard.release();
    for (auto const & i : containers) {
        i->rootsCheck(target);
    }
}

std::shared_ptr<data::StructModelBase> data::SharedStruct::copy() const {
    std::shared_ptr<data::SharedStruct> newCopy {std::make_shared<data::SharedStruct>(_environment)};
    std::shared_lock guard {_mutex}; // for source
    newCopy->_elements = _elements; // shallow copy
    return newCopy;
}

std::shared_ptr<data::ListModelBase> data::SharedList::copy() const {
    std::shared_ptr<data::SharedList> newCopy {std::make_shared<data::SharedList>(_environment)};
    std::shared_lock guard {_mutex}; // for source
    newCopy->_elements = _elements; // shallow copy
    return newCopy;
}

void data::SharedStruct::put(const data::StringOrd handle, const data::StructElement & element) {
    checkedPut(element, [this,handle](auto & el) {
            std::unique_lock guard {_mutex };
            _elements.emplace(handle, el);
        });
}

void data::SharedStruct::put(const std::string_view sv, const data::StructElement & element) {
    data::Handle handle = _environment.stringTable.getOrCreateOrd(std::string(sv));
    put(handle, element);
}

void data::SharedList::put(int32_t idx, const data::StructElement & element) {
    checkedPut(element, [this,idx](auto & el) {
        std::unique_lock guard {_mutex };
        size_t realIdx;
        if (idx < 0) {
            realIdx = _elements.size() + idx;
        } else {
            realIdx = idx;
        }
        if (realIdx > _elements.size()) {
            throw std::out_of_range("Put index out of range");
        } else if (realIdx == _elements.size()) {
            _elements.push_back(el);
        } else {
            _elements[idx] = el;
        }
    });
}

void data::SharedList::insert(int32_t idx, const data::StructElement & element) {
    checkedPut(element, [this,idx](auto & el) {
        std::unique_lock guard {_mutex };
        size_t realIdx;
        if (idx < 0) {
            realIdx = _elements.size() + idx + 1; // -1 inserts at end
        } else {
            realIdx = idx;
        }
        if (realIdx > _elements.size()) {
            throw std::out_of_range("Put index out of range");
        } else if (realIdx == _elements.size()) {
            _elements.push_back(el);
        } else {
            _elements.insert(_elements.begin()+idx, el);
        }
    });
}

bool data::SharedStruct::hasKey(const data::StringOrd handle) const {
    //_environment.stringTable.assertStringHandle(handle);
    std::shared_lock guard {_mutex};
    auto i = _elements.find(handle);
    return i != _elements.end();
}

uint32_t data::SharedList::length() const {
    std::shared_lock guard {_mutex};
    return _elements.size();
}

data::StructElement data::SharedStruct::get(const std::string_view sv) const {
    data::Handle handle = _environment.stringTable.getOrCreateOrd(std::string(sv));
    return get(handle);
}

data::StructElement data::SharedStruct::get(data::StringOrd handle) const {
    //_environment.stringTable.assertStringHandle(handle);
    std::shared_lock guard {_mutex};
    auto i = _elements.find(handle);
    if (i == _elements.end()) {
        return {};
    } else {
        return i->second;
    }
}

data::StructElement data::SharedList::get(int32_t idx) const {
    std::shared_lock guard {_mutex};
    size_t realIdx;
    if (idx < 0) {
        realIdx = _elements.size() + idx;
    } else {
        realIdx = idx;
    }
    if (realIdx >= _elements.size()) {
        return {};
    } else {
        return _elements[idx];
    }
}
