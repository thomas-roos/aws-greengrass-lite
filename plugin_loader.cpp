#include "plugin_loader.h"
#include <iostream>
namespace fs = std::filesystem;

NativePlugin::~NativePlugin() {
    if (_handle) {
        ::dlclose(_handle);
        _handle = nullptr;
    }
}

NativePlugin::NativePlugin(std::string_view name) : _moduleName(name) {
}

void NativePlugin::load(const std::string & filePath) {
    _handle = ::dlopen(filePath.c_str(), RTLD_NOW | RTLD_LOCAL | RTLD_DEEPBIND);
    if (_handle == nullptr) {
        std::string error {dlerror()};
        throw std::runtime_error(std::string("Cannot load shared object: ")+filePath + std::string(" ") + error);
    }
    _lifecycleFn = reinterpret_cast<lifecycleFn_t>(::dlsym(_handle, "greengrass_lifecycle"));
    _initializeFn = reinterpret_cast<initializeFn_t >(::dlsym(_handle, "greengrass_initialize"));
}

void NativePlugin::initialize() {
    // TODO: thread safety?
    if (_initializeFn != nullptr) {
        _initializeFn();
    }
}

void NativePlugin::lifecycle(Handle phase) {
    // TODO: thread safety?
    if (_lifecycleFn != nullptr) {
        _lifecycleFn(phase.asInt());
    }
}

void PluginLoader::discoverPlugins() {
    std::unique_lock guard{_mutex};
    // two-layer iterator just to make testing easier
    for (const auto & top : fs::directory_iterator(".")) {
        if (top.is_regular_file()) {
            discoverPlugin(top);
        } else if (top.is_directory()) {
            for (const auto &fileEnt: fs::directory_iterator(top)) {
                if (fileEnt.is_regular_file()) {
                    discoverPlugin(fileEnt);
                }
            }
        }
    }
}

void PluginLoader::discoverPlugin(const fs::directory_entry &entry) {
    std::string name {entry.path()};
    if (endsWith(name, ".so")) {
        loadNativePlugin(name);
    }
}

void PluginLoader::loadNativePlugin(const std::string &name) {
    // Linux/Unix specific
    std::shared_ptr<NativePlugin> plugin {std::make_shared<NativePlugin>(name)};
    plugin->load(name);

    //std::unique_lock guard{_mutex};
    _plugins.push_back(plugin);
}

std::vector<std::shared_ptr<AbstractPlugin>> PluginLoader::pluginSnapshot() {
    std::shared_lock guard{_mutex};
    std::vector<std::shared_ptr<AbstractPlugin>> copy;
    copy.reserve(_plugins.size());
    for (const auto &i : _plugins) {
        copy.push_back(i);
    }
    return copy;
}

void PluginLoader::lifecycleStart() {
    Handle key = _environment.stringTable.getOrCreateOrd("start");
    lifecycle(key);
}

void PluginLoader::lifecycleRun() {
    Handle key = _environment.stringTable.getOrCreateOrd("run");
    lifecycle(key);
}

void PluginLoader::initialize() {
    for (const auto &i : pluginSnapshot()) {
        i->initialize();
    }
}

void PluginLoader::lifecycle(Handle handle) {
    for (const auto &i : pluginSnapshot()) {
        i->lifecycle(handle);
    }
}
