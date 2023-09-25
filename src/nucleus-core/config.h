#pragma once

#include "shared_struct.h"
#include "handle_table.h"
#include "string_table.h"
#include "expire_time.h"

namespace config {
    class Timestamp;
    class Element;
    class Topic;
    class Topics;

    //
    // Greengrass-Java use of config timestamp can be considered to be a signed long representing milliseconds since
    // epoch. Given the special constants, it's better to handle as 64-bit signed integer rather than handle all
    // the weird edge conditions.
    //
    class Timestamp {
    private:
        int64_t _time; // since epoch
    public:
        constexpr Timestamp() : _time{0} {
        }

        explicit constexpr Timestamp(int64_t timeMillis) : _time{timeMillis} {
        }

        constexpr Timestamp(const Timestamp &time) : _time{time._time} {
        }

        explicit constexpr Timestamp(const std::chrono::time_point<std::chrono::system_clock> time) :
                _time(static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()).count())) {
        }

        static Timestamp now() {
            return Timestamp{std::chrono::system_clock::now()};
        }

        [[nodiscard]] constexpr int64_t asMilliseconds() const {
            return _time;
        };

        constexpr bool operator==(const Timestamp &other) const {
            return _time == other._time;
        }

        constexpr Timestamp &operator=(const Timestamp &time) {
            _time = time._time;
            return *this;
        }

        static constexpr Timestamp never();
        static constexpr Timestamp dawn();
        static constexpr Timestamp infinite();
    };

    constexpr Timestamp Timestamp::never() {
        return Timestamp{0};
    }
    constexpr Timestamp Timestamp::dawn() {
        return Timestamp{1};
    }
    constexpr Timestamp Timestamp::infinite() {
        return Timestamp{-1};
    }

    // Extend structure element to include name & time
    // not entirely parallels GG-Java "Topic"
    class Element : public StructElement {
    private:
        static int foldChar(int c);

    protected:
        Handle _nameOrd;
        Timestamp _modtime;

    public:
        Element() = default;
        Element(const Element & el) = default;
        explicit Element(const StructElement & se) : StructElement(se) {
        }
        explicit Element(Handle ord, const StructElement & se) : StructElement(se),
            _nameOrd{ord} {
        }
        explicit Element(Handle ord, const Timestamp& timestamp) :
            _nameOrd{ord}, _modtime{timestamp} {
        }
        explicit Element(Handle ord, const Timestamp& timestamp, std::shared_ptr<Topics> topics);
        Element & operator=(const Element & other) = default;
        static const Element nullElement;

        Handle getOrd() {
            return _nameOrd;
        }
        Timestamp getModTime() {
            return _modtime;
        }
        Element withOrd(Handle ord) {
            Element copy {*this};
            copy._nameOrd = ord;
            return copy;
        }
        Element withName(Environment & env, std::string str);
        Element withModTime(const Timestamp& modTime) {
            Element copy {*this};
            copy._modtime = modTime;
            return copy;
        }
        [[nodiscard]] Handle getKey(Environment & env) const;
        static Handle getKey(Environment & env, Handle ord);

        [[nodiscard]] StructElement slice() {
            return StructElement(_value);
        }
        [[nodiscard]] bool isTopics() const {
            if (!isStruct()) {
                return false;
            }
            std::shared_ptr<Topics> t = std::dynamic_pointer_cast<Topics>(getStructRef());
            return static_cast<bool>(t);
        }

        [[nodiscard]] std::shared_ptr<Topics> getTopicsRef() const {
            if (!isTopics()) {
                throw std::runtime_error("Child is not a topic");
            }
            return std::static_pointer_cast<Topics>(getStructRef());
        }
    };

    inline const Element Element::nullElement {};

    //
    // Proxy to achieve a pattern similar to GG-Java "Topic"
    //

    class Topic {
    private:
        Element _element;
        std::shared_ptr<Topics> _owner;
    public:
        Topic(const Topic & other) :
                _owner(other._owner), _element(other._element) {
        }
        explicit Topic(Topic * other) :
                _owner(other->_owner), _element(other->_element) {
        }
        Topic(std::shared_ptr<Topics> owner, const Element & element) :
                _owner(std::move(owner)), _element(element) {
        }
        Element & get() {
            return _element;
        }
        void update();
    };

    // Set of key/value pairs
    // not entirely parallels GG-Java "Topics"
    class Topics : public Structish {
    protected:
        std::weak_ptr<Topics> _parent;
        std::map<Handle, Element, Handle::CompLess> _children;
        mutable std::shared_mutex _mutex;
        bool putStruct(Handle key, const Element & element);
        void rootsCheck(const Structish * target) const override;

    public:
        explicit Topics(Environment & environment, const std::shared_ptr<Topics> & parent) :
            Structish{environment}, _parent{parent} {
        }
        std::shared_ptr<Topics> topics_shared_from_this() {
            return std::dynamic_pointer_cast<Topics>(shared_from_this());
        }

        void put(Handle handle, const StructElement & element) override;
        void put(std::string_view sv, const StructElement & element) override;
        void updateChild(const Element & element);
        bool hasKey(Handle handle) override;
        StructElement get(Handle handle) const override;
        StructElement get(std::string_view name) const override;
        std::shared_ptr<Structish> copy() const override;
        Element createChild(Handle nameOrd, const std::function<Element(Handle)> & creator);
        std::unique_ptr<Topic> createLeafChild(Handle nameOrd, const Timestamp & timestamp = Timestamp());
        std::unique_ptr<Topic> createLeafChild(std::string_view name, const Timestamp & timestamp = Timestamp());
        std::shared_ptr<Topics> createInteriorChild(Handle nameOrd, const Timestamp & timestamp = Timestamp::now());
        std::shared_ptr<Topics> createInteriorChild(std::string_view name, const Timestamp & timestamp = Timestamp::now());
        Element getChild(Handle handle) const;
        std::unique_ptr<Topic> findLeafChild(Handle handle);
        std::unique_ptr<Topic> findLeafChild(std::string_view name);
        std::shared_ptr<Topics> findInteriorChild(Handle nameOrd);
        std::shared_ptr<Topics> findInteriorChild(std::string_view name);
        size_t getSize() const;
    };

    inline Element::Element(Handle ord, const config::Timestamp &timestamp, std::shared_ptr<Topics> topics) :
            StructElement(std::static_pointer_cast<Structish>(topics)), _nameOrd(ord), _modtime(timestamp) {
    }

    inline void Topic::update() {
        _owner->updateChild(_element);
    }

    class Manager {
    private:
        Environment & _environment;
        std::shared_ptr<Topics> _root;
    public:
        explicit Manager(Environment & environment) :
            _environment{environment},
            _root{std::make_shared<Topics>(environment, nullptr)} {
        }
        std::shared_ptr<Topics> root() {
            return _root;
        }
    };
}
