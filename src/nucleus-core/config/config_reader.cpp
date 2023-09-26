#include "config_reader.h"
#include "../data/environment.h"
#include <iostream>
#include <fstream>
#include <memory>
#include <filesystem>

namespace config {
    void YamlReader::read(const std::filesystem::path &path) {
        //
        // yaml-cpp has a number of flaws, but short of rewriting a YAML parser, is sufficient to get going
        //
        std::ifstream stream{path};
        if (!stream) {
            throw std::runtime_error("Unable to read config file");
        }
        YAML::Node root = YAML::Load(stream);
        inplaceMap(_target, root);
    }

    void YamlReader::inplaceMap(const std::shared_ptr<Topics> & topics, YAML::Node & node) {
        if (!node.IsMap()) {
            throw std::runtime_error("Expecting a map");
        }
        for (auto i : node) {
            auto key = i.first.as<std::string>();
            inplaceValue(topics, key, i.second);
        }
    }

    void YamlReader::inplaceValue(const std::shared_ptr<Topics> & topics, const std::string & key, YAML::Node & node) {
        switch (node.Type()) {
            case YAML::NodeType::Map:
                nestedMapValue(topics, key, node);
                break;
            case YAML::NodeType::Sequence:
                inplaceSequenceValue(topics, key, node);
                break;
            case YAML::NodeType::Scalar:
                inplaceScalarValue(topics, key, node);
                break;
            case YAML::NodeType::Null:
                inplaceNullValue(topics, key);
                break;
            default:
                // ignore anything else
                break;
        }
    }

    void YamlReader::inplaceNullValue(const std::shared_ptr<Topics> & topics, const std::string & key) {
        topics->createLeafChild(key, _timestamp);
    }

    void YamlReader::inplaceScalarValue(const std::shared_ptr<Topics> & topics, const std::string & key, YAML::Node & node) {
        std::unique_ptr<Topic> topic = topics->createLeafChild(key, _timestamp);
        topic->get().setModTime(_timestamp).value() = node.as<std::string>();
        topic->update();
    }

    void YamlReader::nestedMapValue(const std::shared_ptr<Topics> &topics, const std::string &key, YAML::Node &node) {
        std::shared_ptr<Topics> nested = topics->createInteriorChild(key, _timestamp);
        inplaceMap(nested, node);
    }

    void YamlReader::inplaceSequenceValue(const std::shared_ptr<Topics> &topics, const std::string &key,
                                        YAML::Node &node) {
        throw std::runtime_error("Cannot handle sequences yet");
    }
}
