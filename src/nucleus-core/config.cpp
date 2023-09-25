#include "config.h"
#include "environment.h"

//
// TODO: Decide if this belongs in Nucleus, or belongs in another plugin
// Note that config intake is case insensitive - config comes from
// a settings file (YAML), transaction log (YAML), or cloud (JSON or YAML)
// For optimization, this implementation assumes all config keys are stored lower-case
// which means translation on intake is important
//
namespace config {

    int Element::foldChar(int c) {
        // simple folding rules to handle the mixed-ascii-case config style
        if (c >= 'A' && c <= 'Z') {
            return c-'A'+'a';
        } else {
            return c;
        }
    }
    Handle Element::getKey(Environment & env) const {
        return getKey(env, _nameOrd);
    }
    Handle Element::getKey(Environment & env, Handle nameOrd) {
        if (!nameOrd) {
            return nameOrd;
        }
        std::string str = env.stringTable.getString(nameOrd);
        std::string folded;
        // a folded string strictly acts on the ascii range and not on international characters
        // this keeps it predictable and handles the problems with GG configs
        std::transform(str.begin(), str.end(), folded.begin(), foldChar);
        if (str == folded) {
            return nameOrd;
        } else {
            return env.stringTable.getOrCreateOrd(folded);
        }
    }

    Element Element::withName(Environment &env, std::string str) {
        return withOrd(env.stringTable.getOrCreateOrd(str));
    }

    bool Topics::putStruct(Handle key, const Element &element) {
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
        std::unique_lock cycleGuard{_environment.cycleCheckMutex};
        otherStruct->rootsCheck(this);
        _children[key] = element;

        return true;
    }

    void Topics::updateChild(const Element &element) {
        Handle key = element.getKey(_environment);
        if (!(element.isStruct() && putStruct(key, element))) {
            std::unique_lock guard{_mutex};
            _children[key] = element;
        }
    }

    void Topics::rootsCheck(const Structish *target) const { // NOLINT(*-no-recursion)
        if (this == target) {
            throw std::runtime_error("Recursive reference of structure");
        }
        // we don't want to keep nesting locks else we will deadlock
        std::shared_lock guard{_mutex};
        std::vector<std::shared_ptr<Structish>> structs;
        for (auto const &i: _children) {
            if (i.second.isStruct()) {
                std::shared_ptr<Structish> otherStruct = i.second.getStructRef();
                if (otherStruct) {
                    structs.emplace_back(otherStruct);
                }
            }
        }
        guard.release();
        for (auto const &i: structs) {
            i->rootsCheck(target);
        }
    }

    std::shared_ptr<Structish> Topics::copy() const {
        const std::shared_ptr<Topics> parent {_parent};
        std::shared_ptr<Topics> newCopy{std::make_shared<Topics>(_environment, parent)};
        std::shared_lock guard{_mutex}; // for source
        for (auto const &i: _children) {
            newCopy->put(i.first, i.second);
        }
        return newCopy;
    }

    void Topics::put(const Handle handle, const StructElement &element) {
        updateChild(Element{handle, element});
    }

    void Topics::put(const std::string_view sv, const StructElement &element) {
        Handle handle = _environment.stringTable.getOrCreateOrd(std::string(sv));
        put(handle, element);
    }

    bool Topics::hasKey(const Handle handle) {
        //_environment.stringTable.assertStringHandle(handle);
        Handle key = Element::getKey(_environment, handle);
        std::shared_lock guard{_mutex};
        auto i = _children.find(key);
        return i != _children.end();
    }

    Element Topics::createChild(Handle nameOrd, const std::function<Element(Handle)> & creator) {
        Handle key = Element::getKey(_environment, nameOrd);
        std::unique_lock guard{_mutex};
        auto i = _children.find(key);
        if (i != _children.end()) {
            return i->second;
        } else {
            return _children[key] = creator(nameOrd);
        }
    }

    std::unique_ptr<Topic> Topics::createLeafChild(Handle nameOrd, const Timestamp & timestamp) {
        Element leaf = createChild(nameOrd, [&](auto ord) {
            return Element(ord, timestamp);
        });
        if (leaf.isStruct()) {
            throw std::runtime_error("Not a leaf node");
        }
        return std::make_unique<Topic>(topics_shared_from_this(), leaf);
    }

    std::unique_ptr<Topic> Topics::createLeafChild(std::string_view sv, const Timestamp & timestamp) {
        Handle handle = _environment.stringTable.getOrCreateOrd(std::string(sv));
        return createLeafChild(handle, timestamp);
    }

    std::shared_ptr<Topics> Topics::createInteriorChild(Handle nameOrd, const Timestamp & timestamp) {
        Element leaf = createChild(nameOrd, [&](auto ord) {
            std::shared_ptr<Topics> parent {topics_shared_from_this()};
            std::shared_ptr<Topics> nested {std::make_shared<Topics>(_environment, parent)};
            return Element(ord, timestamp, nested);
        });
        return leaf.getTopicsRef();
    }

    std::shared_ptr<Topics> Topics::createInteriorChild(std::string_view sv, const Timestamp & timestamp) {
        Handle handle = _environment.stringTable.getOrCreateOrd(std::string(sv));
        return createInteriorChild(handle, timestamp);
    }

    Element Topics::getChild(Handle handle) const {
        //_environment.stringTable.assertStringHandle(handle);
        Handle key = Element::getKey(_environment, handle);
        std::shared_lock guard{_mutex};
        auto i = _children.find(key);
        if (i != _children.end()) {
            return i->second;
        } else {
            return Element::nullElement;
        }
    }

    std::unique_ptr<Topic> Topics::findLeafChild(Handle handle) {
        Element leaf = getChild(handle);
        if (leaf && !leaf.isTopics()) {
            return std::make_unique<Topic>(topics_shared_from_this(), leaf);
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<Topic> Topics::findLeafChild(std::string_view name) {
        Handle handle = _environment.stringTable.getOrCreateOrd(std::string(name));
        return findLeafChild(handle);
    }

    std::shared_ptr<Topics> Topics::findInteriorChild(Handle handle) {
        Element leaf = getChild(handle);
        if (leaf && leaf.isTopics()) {
            return leaf.getTopicsRef();
        } else {
            return nullptr;
        }
    }

    std::shared_ptr<Topics> Topics::findInteriorChild(std::string_view name) {
        Handle handle = _environment.stringTable.getOrCreateOrd(std::string(name));
        return findInteriorChild(handle);
    }

    StructElement Topics::get(Handle handle) const {
        return getChild(handle).slice();
    }

    StructElement Topics::get(const std::string_view sv) const {
        Handle handle = _environment.stringTable.getOrCreateOrd(std::string(sv));
        return get(handle);
    }

    size_t Topics::getSize() const {
        std::shared_lock guard{_mutex};
        return _children.size();
    }
}
