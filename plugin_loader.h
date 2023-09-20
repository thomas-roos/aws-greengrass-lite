#pragma once
#include <memory>
#include <string>
#include <list>
#include <shared_mutex>
#include <mutex>
#include <filesystem>
#include "environment.h"
#if defined(USE_DLFCN)
#include <dlfcn.h>
#elif defined(USE_WINDLL)
#include <windows.h>
#endif

class AbstractPlugin : public std::enable_shared_from_this<AbstractPlugin> {
public:
    virtual ~AbstractPlugin() = default;
    virtual void initialize() = 0;
    virtual void lifecycle(Handle phase) = 0;
};

class NativePlugin : public AbstractPlugin {
private:
    std::string _moduleName;
#if defined(USE_DLFCN)
    void * _handle { nullptr };
    typedef void (*initializeFn_t)();
    typedef void (*lifecycleFn_t)(uint32_t phase);
#elif defined(USE_WINDLL)
    HINSTANCE _handle { nullptr };
    typedef void (WINAPI *initializeFn_t)();
    typedef void (WINAPI *lifecycleFn_t)(uint32_t phase);

#endif
    initializeFn_t _initializeFn { nullptr };
    lifecycleFn_t _lifecycleFn { nullptr };

public:
    explicit NativePlugin(std::string_view name);
    ~NativePlugin() override;
    std::shared_ptr<NativePlugin> shared_from_this() {
        return std::static_pointer_cast<NativePlugin>(AbstractPlugin::shared_from_this());
    }
    void load(const std::string & filePath);
    void initialize() override;
    void lifecycle(Handle phase) override;
};

class PluginLoader {
private:
    Environment & _environment;
    std::list<std::shared_ptr<AbstractPlugin>> _plugins;
    std::shared_mutex _mutex;

    static bool endsWith(const std::string_view name, const std::string_view suffix) {
        if (name.length() <= suffix.length()) {
            return false;
        }
        return name.compare(name.length()-suffix.length(), suffix.length(), suffix) == 0;
    }

    std::vector<std::shared_ptr<AbstractPlugin>> pluginSnapshot();

public:
    explicit PluginLoader(Environment & environment) : _environment{environment} {
    }
    PluginLoader() : PluginLoader(Environment::singleton()) {
    }

    void discoverPlugins();
    void discoverPlugin(const std::filesystem::directory_entry &entry);

    void loadNativePlugin(const std::string &name);

    void initialize();

    void lifecycleStart();

    void lifecycleRun();

    void lifecycle(Handle phase);
};
