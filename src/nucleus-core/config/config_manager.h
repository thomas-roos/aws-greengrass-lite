#pragma once

#include "data/shared_struct.h"
#include "data/handle_table.h"
#include "data/string_table.h"
#include "tasks/expire_time.h"
#include "data/safe_handle.h"
#include <filesystem>
#include <utility>

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
        constexpr Timestamp(const Timestamp &time) = default;
        constexpr Timestamp(Timestamp &&time) = default;
        constexpr Timestamp() : _time{0} {
        }
        explicit constexpr Timestamp(int64_t timeMillis) : _time{timeMillis} {
        }
        explicit constexpr Timestamp(const std::chrono::time_point<std::chrono::system_clock> time) :
                _time(static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()).count())) {
        }
        ~Timestamp() = default;




        static Timestamp now() {
            return Timestamp{std::chrono::system_clock::now()};
        }

        [[nodiscard]] constexpr int64_t asMilliseconds() const {
            return _time;
        };

        constexpr bool operator==(const Timestamp &other) const {
            return _time == other._time;
        }

        constexpr Timestamp &operator=(const Timestamp &time) = default;
        constexpr Timestamp &operator=(Timestamp &&time) = default;

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
    class Element : public data::StructElement {
    protected:
        data::Handle _nameOrd;
        Timestamp _modtime;

    public:
        Element() = default;
        Element(const Element & el) = default;
        Element(Element && el) = default;
        explicit Element(const StructElement & se) : data::StructElement(se) {
        }
        explicit Element(data::Handle ord, const StructElement & se) : data::StructElement(se),
                                                                       _nameOrd{ord} {
        }
        explicit Element(data::Handle ord, const Timestamp& timestamp) :
            _nameOrd{ord}, _modtime{timestamp} {
        }
        explicit Element(data::Handle ord, const Timestamp& timestamp, ValueType newVal) :
                StructElement(std::move(newVal)), _nameOrd{ord}, _modtime{timestamp} {
        }
        explicit Element(data::Handle ord, const Timestamp& timestamp, const std::shared_ptr<Topics>& topics);
        ~Element() = default;
        Element & operator=(const Element & other) = default;
        Element & operator=(Element && other) = default;
        static const Element nullElement;

        data::Handle getOrd() {
            return _nameOrd;
        }
        Timestamp getModTime() {
            return _modtime;
        }
        data::StructElement::ValueType & value() {
            return _value;
        }

        Element & setOrd(data::Handle ord) {
            _nameOrd = ord;
            return *this;
        }
        Element & setName(data::Environment & env, const std::string & str);
        Element & setModTime(const Timestamp& modTime) {
            _modtime = modTime;
            return *this;
        }
        [[nodiscard]] data::Handle getKey(data::Environment & env) const;
        static data::Handle getKey(data::Environment & env, data::Handle ord);

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

    inline const Element Element::nullElement {}; // NOLINT(cert-err58-cpp)

    //
    // Proxy to achieve a pattern similar to GG-Java "Topic"
    //

    class Topic {
    private:
        Element _element;
        std::shared_ptr<Topics> _owner;
    public:
        Topic(const Topic & other) = default;
        Topic(Topic && other) = default;
        explicit Topic(Topic * other) :
                _owner(other->_owner), _element(other->_element) {
        }
        Topic(std::shared_ptr<Topics> owner, Element  element) :
                _owner(std::move(owner)), _element(std::move(element)) {
        }
        ~Topic() = default;
        Topic & operator=(const Topic &) = default;
        Topic & operator=(Topic &&) = default;
        Element & get() {
            return _element;
        }
        void update();
    };

    // Set of key/value pairs
    // not entirely parallels GG-Java "Topics"
    class Topics : public data::Structish {
    protected:
        std::weak_ptr<Topics> _parent;
        std::map<data::Handle, Element, data::Handle::CompLess> _children;
        mutable std::shared_mutex _mutex;
        bool putStruct(data::Handle key, const Element & element);
        void rootsCheck(const data::Structish * target) const override;

    public:
        explicit Topics(data::Environment & environment, const std::shared_ptr<Topics> & parent) :
                data::Structish{environment}, _parent{parent} {
        }
        std::shared_ptr<Topics> topics_shared_from_this() {
            return std::dynamic_pointer_cast<Topics>(shared_from_this());
        }

        void put(data::Handle handle, const data::StructElement & element) override;
        void put(std::string_view sv, const data::StructElement & element) override;
        void updateChild(const Element & element);
        bool hasKey(data::Handle handle) override;
        data::StructElement get(data::Handle handle) const override;
        data::StructElement get(std::string_view name) const override;
        std::shared_ptr<data::Structish> copy() const override;
        Element createChild(data::Handle nameOrd, const std::function<Element(data::Handle)> & creator);
        std::unique_ptr<Topic> createLeafChild(data::Handle nameOrd, const Timestamp & timestamp = Timestamp());
        std::unique_ptr<Topic> createLeafChild(std::string_view name, const Timestamp & timestamp = Timestamp());
        std::shared_ptr<Topics> createInteriorChild(data::Handle nameOrd, const Timestamp & timestamp = Timestamp::now());
        std::shared_ptr<Topics> createInteriorChild(std::string_view name, const Timestamp & timestamp = Timestamp::now());
        Element getChild(data::Handle handle) const;
        std::unique_ptr<Topic> findLeafChild(data::Handle handle);
        std::unique_ptr<Topic> findLeafChild(std::string_view name);
        std::shared_ptr<Topics> findInteriorChild(data::Handle nameOrd) const;
        std::shared_ptr<Topics> findInteriorChild(std::string_view name);
        size_t getSize() const;
    };

    inline Element::Element(data::Handle ord, const config::Timestamp &timestamp, const std::shared_ptr<Topics>& topics) :
            data::StructElement(std::static_pointer_cast<data::Structish>(topics)), _nameOrd(ord), _modtime(timestamp) {
    }

    inline void Topic::update() {
        _owner->updateChild(_element);
    }

    class Manager {
    private:
        data::Environment & _environment;
        std::shared_ptr<Topics> _root;
    public:
        explicit Manager(data::Environment & environment) :
            _environment{environment},
            _root{std::make_shared<Topics>(environment, nullptr)} {
        }
        std::shared_ptr<Topics> root() {
            return _root;
        }
        void read(std::filesystem::path path);
    };
}
