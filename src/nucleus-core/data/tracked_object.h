#pragma once
#include "safe_handle.h"
#include <util.h>
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
    class TrackingScope;

    //
    // Base class for all objects that can be tracked with one or more handles
    // The object lives as long as there is one or more handles, or if there is
    // one or more std::shared_ptr<> reference to the object
    //
    class TrackedObject : public util::RefObject<TrackedObject> {
    protected:
        Environment & _environment;
    public:
        TrackedObject(const TrackedObject&) = delete;
        TrackedObject(TrackedObject&&) noexcept = default;
        TrackedObject & operator=(const TrackedObject&) = delete;
        TrackedObject & operator=(TrackedObject&&) noexcept = delete;
        virtual ~TrackedObject() = default;
        explicit TrackedObject(Environment & environment) : _environment {environment} {
        }
    };

    //
    // TODO: changing
    //
    class ObjectAnchor : public util::RefObject<ObjectAnchor> {
        friend TrackingScope;
    protected:
        Environment & _environment;
        std::weak_ptr<TrackingScope> _owner;
    private:
        Handle _handle {Handle::nullHandle()};
        std::shared_ptr<TrackedObject> _object; // multiple anchors to one object
    public:
        friend class HandleTable;

        explicit ObjectAnchor(Environment & environment, std::shared_ptr<TrackedObject> && obj)
                : _environment {environment}, _object {obj} {
        }

        explicit ObjectAnchor(Environment & environment, TrackedObject * obj)
                : _environment {environment}, _object {obj->shared_from_this()} {
        }

        virtual ~ObjectAnchor();
        Handle getHandle();

        template<typename T>
        std::shared_ptr<T> getObject() {
            std::shared_ptr<T> ptr { std::dynamic_pointer_cast<T>(_object) };
            if (!ptr) {
                throw std::bad_cast();
            }
            return ptr;
        }

        std::shared_ptr<TrackingScope> getOwner() {
            return _owner.lock();
        }

        virtual bool release();
    };

    //
    // A TrackedObject that represents a scope (Task, etc.)
    // ensuring all handles associated with that scope will be removed when that scope exits.
    //
    class TrackingScope : public TrackedObject {
    protected:
        std::map<Handle, std::shared_ptr<ObjectAnchor>, Handle::CompLess> _roots;
        mutable std::shared_mutex _mutex;
    public:
        explicit TrackingScope(Environment & environment) : TrackedObject{environment} {
        }

        std::shared_ptr<ObjectAnchor> anchor(TrackedObject * obj);
        std::shared_ptr<ObjectAnchor> anchor(ObjectAnchor * anchor);
        std::shared_ptr<ObjectAnchor> anchor(Handle handle);
        std::shared_ptr<ObjectAnchor> anchor(const std::shared_ptr<ObjectAnchor>& anchored);
        bool release(ObjectAnchor * anchored);
        bool release(Handle handle);
        bool release(std::shared_ptr<ObjectAnchor> &anchored);
    };

} // data
