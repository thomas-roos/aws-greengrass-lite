#include "plugin_loader.h"
#include "shared_struct.h"
#include "task.h"
namespace fs = std::filesystem;

NativePlugin::~NativePlugin() {
#if defined(USE_DLFCN)
    if (_handle) {
        ::dlclose(_handle);
        _handle = nullptr;
    }
#elif defined(USE_WINDLL)
    if (_handle) {
        ::FreeLibrary(_handle);
        _handle = nullptr;
    }
#endif
}

void NativePlugin::load(const std::string & filePath) {
#if defined(USE_DLFCN)
    nativeHandle_t handle = ::dlopen(filePath.c_str(), RTLD_NOW | RTLD_LOCAL | RTLD_DEEPBIND);
    _handle = handle;
    if (handle == nullptr) {
        std::string error {dlerror()};
        throw std::runtime_error(std::string("Cannot load shared object: ")+filePath + std::string(" ") + error);
    }
    _lifecycleFn = reinterpret_cast<lifecycleFn_t>(::dlsym(_handle, "greengrass_lifecycle"));
#elif defined(USE_WINDLL)
    nativeHandle_t handle = ::LoadLibraryEx(filePath.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    _handle = handle;
    if (handle == nullptr) {
        uint32_t lastError = ::GetLastError();
        // TODO: use FormatMessage
        throw std::runtime_error(std::string("Cannot load DLL: ")+filePath + std::string(" ") + std::to_string(lastError));
    }
    _lifecycleFn = reinterpret_cast<lifecycleFn_t>(::GetProcAddress(_handle, "greengrass_lifecycle"));
#endif
}

bool NativePlugin::isActive() {
    return _lifecycleFn;
}

void PluginLoader::lifecycle(Handle phase, const std::shared_ptr<Structish> & data) {
    // TODO: Run this inside of a task, right now we end up using calling threads task
    // However probably good to defer that until there's a basic lifecycle manager
    for (const auto &i : getPlugins()) {
        std::shared_ptr<AbstractPlugin> plugin {i->getObject<AbstractPlugin>()};
        if (plugin->isActive()) {
            plugin->lifecycle(Handle{i}, phase, data);
        }
    }
}

void NativePlugin::lifecycle(Handle pluginAnchor, Handle phase, const std::shared_ptr<Structish> & data) {
    lifecycleFn_t lifecycleFn = _lifecycleFn;
    if (lifecycleFn != nullptr) {
        std::shared_ptr<Structish> copy = data->copy();
        std::shared_ptr<Task> threadTask = _environment.handleTable.getObject<Task>(Task::getThreadSelf());
        std::shared_ptr<Anchored> dataAnchor = threadTask->anchor(copy.get());
        lifecycleFn(
                pluginAnchor.asInt(),
                phase.asInt(),
                Handle {dataAnchor}.asInt()
                );
    }
}

void DelegatePlugin::lifecycle(Handle pluginAnchor, Handle phase, const std::shared_ptr<Structish> & data) {
    uintptr_t delegateContext;
    ggapiLifecycleCallback delegateLifecycle;
    {
        std::shared_lock guard{_mutex};
        delegateContext = _delegateContext;
        delegateLifecycle = _delegateLifecycle;
    }
    if (delegateLifecycle != nullptr) {
        std::shared_ptr<Structish> copy = data->copy();
        std::shared_ptr<Task> threadTask = _environment.handleTable.getObject<Task>(Task::getThreadSelf());
        std::shared_ptr<Anchored> dataAnchor = threadTask->anchor(copy.get());
        delegateLifecycle(
                delegateContext,
                pluginAnchor.asInt(),
                phase.asInt(),
                Handle {dataAnchor}.asInt()
        );
    }
}

void PluginLoader::discoverPlugins() {
    // two-layer iterator just to make testing easier
    fs::path root = fs::absolute(".");
    for (const auto & top : fs::directory_iterator(root)) {
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
    std::string name {entry.path().generic_string()};
#if defined(NATIVE_SUFFIX)
    if (entry.path().extension().compare(NATIVE_SUFFIX) == 0) {
        loadNativePlugin(name);
        return;
    }
#endif
}

void PluginLoader::loadNativePlugin(const std::string &name) {
    std::shared_ptr<NativePlugin> plugin {std::make_shared<NativePlugin>(_environment, name)};
    plugin->load(name);
    // add the plugin to collection by "anchoring"
    // which solves a number of interesting problems
    anchor(plugin.get());
}

std::vector<std::shared_ptr<Anchored>> PluginLoader::getPlugins() {
    std::shared_lock guard{_mutex};
    std::vector<std::shared_ptr<Anchored>> copy;
    copy.reserve(_roots.size());
    for (const auto &i : _roots) {
        copy.emplace_back(i.second);
    }
    return copy;
}

void PluginLoader::lifecycleBootstrap(const std::shared_ptr<Structish> & data) {
    Handle key = _environment.stringTable.getOrCreateOrd("bootstrap");
    lifecycle(key, data);
}

void PluginLoader::lifecycleDiscover(const std::shared_ptr<Structish> & data) {
    Handle key = _environment.stringTable.getOrCreateOrd("discover");
    lifecycle(key, data);
}

void PluginLoader::lifecycleStart(const std::shared_ptr<Structish> & data) {
    Handle key = _environment.stringTable.getOrCreateOrd("start");
    lifecycle(key, data);
}

void PluginLoader::lifecycleRun(const std::shared_ptr<Structish> & data) {
    Handle key = _environment.stringTable.getOrCreateOrd("run");
    lifecycle(key, data);
}

void PluginLoader::lifecycleTerminate(const std::shared_ptr<Structish> & data) {
    Handle key = _environment.stringTable.getOrCreateOrd("run");
    lifecycle(key, data);
}
