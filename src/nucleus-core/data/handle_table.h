#pragma once
#include "safe_handle.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <map>
#include <shared_mutex>
#include <mutex>
#include <memory>

namespace data {

    class Environment;
    class HandleTable;
    class AnchoredWithRoots;

    class AnchoredObject : public std::enable_shared_from_this<AnchoredObject> {
    protected:
        Environment & _environment;
    public:
        explicit AnchoredObject(Environment & environment) : _environment {environment} {
        }
        virtual ~AnchoredObject() = default;

    };

    class Anchored : public std::enable_shared_from_this<Anchored> {
        friend AnchoredWithRoots;
    protected:
        Environment & _environment;
        std::weak_ptr<AnchoredWithRoots> _owner;
    private:
        Handle _handle {Handle::nullHandle};
        std::shared_ptr<AnchoredObject> _object; // multiple anchors to one object
    public:
        friend class HandleTable;

        explicit Anchored(Environment & environment, std::shared_ptr<AnchoredObject> && obj)
            : _environment {environment}, _object {obj} {
        }

        explicit Anchored(Environment & environment, AnchoredObject * obj)
            : _environment {environment}, _object {obj->shared_from_this()} {
        }

        virtual ~Anchored();
        Handle getHandle();

        template<typename T>
        std::shared_ptr<T> getObject() {
            return std::dynamic_pointer_cast<T>(_object);
        }

        std::weak_ptr<AnchoredWithRoots> getOwner() {
            return _owner;
        }

        bool release();
    };

    class AnchoredWithRoots : public AnchoredObject {
    protected:
        std::map<Handle, std::shared_ptr<Anchored>, Handle::CompLess> _roots;
        mutable std::shared_mutex _mutex;
    public:
        explicit AnchoredWithRoots(Environment & environment) : AnchoredObject{environment} {
        }

        std::shared_ptr<Anchored> anchor(AnchoredObject * obj);
        std::shared_ptr<Anchored> anchor(Anchored * anchor);
        std::shared_ptr<Anchored> anchor(Handle handle);
        std::shared_ptr<Anchored> anchor(const std::shared_ptr<Anchored>& anchored);
        bool release(Anchored * anchored);
        bool release(Handle handle);
        bool release(std::shared_ptr<Anchored> &anchored);
    };

    //
    // Handle tracking
    //
    class HandleTable {
    private:
        mutable std::shared_mutex _mutex; // shared_mutex would be better
        uint32_t _counter {0};
        std::unordered_map<Handle, std::weak_ptr<Anchored>, Handle::Hash, Handle::CompEq> _handles;

    public:
        Handle getExistingHandle(const Anchored * anchored) const {
            std::shared_lock guard {_mutex};
            return anchored->_handle;
        }

        Handle getHandle(Anchored * anchored);
        void release(Anchored * anchored);

        std::shared_ptr<Anchored> getAnchor(const Handle handle) {
            std::shared_lock guard {_mutex};
            auto i = _handles.find(handle);
            if (i == _handles.end()) {
                return nullptr;
            }
            return i->second.lock();
        }

        template<typename T>
        std::shared_ptr<T> getObject(const Handle handle) {
            std::shared_ptr<Anchored> obj {getAnchor(handle)};
            if (obj) {
                return obj->getObject<T>();
            } else {
                return nullptr;
            }
        }

    };
}

