#pragma once
#include <memory>
#include <string>
#include <list>
#include <shared_mutex>
#include <mutex>
#include <filesystem>
#include <c_api.h>
#include "environment.h"
#include "handle_table.h"
#if defined(USE_DLFCN)
#include <dlfcn.h>
#elif defined(USE_WINDLL)
#include <windows.h>
#endif

class Structish;
//
// Abstract plugin also acts as a global anchor for the given plugin module
//
class AbstractPlugin : public AnchoredWithRoots {
protected:
    std::string _moduleName;
public:
    explicit AbstractPlugin(Environment & environment, const std::string_view & name) :
        AnchoredWithRoots(environment), _moduleName{name} {
    }
    virtual void lifecycle(Handle pluginRoot, Handle phase, const std::shared_ptr<Structish> & data) = 0;
    virtual bool isActive() {
        return true;
    }
};

//
// Delegate plugin is managed by a parent plugin
//
class DelegatePlugin : public AbstractPlugin {
private:
    std::shared_ptr<AbstractPlugin> _parent; // delegate keeps parent in memory
    ggapiLifecycleCallback _delegateLifecycle { nullptr }; // called to handle this delegate
    uintptr_t _delegateContext {0}; // use of this defined by delegate
public:
    explicit DelegatePlugin(Environment & environment, std::string_view name,
                            const std::shared_ptr<AbstractPlugin> & parent,
                            ggapiLifecycleCallback delegateLifecycle,
                            uintptr_t delegateContext) :
        AbstractPlugin(environment, name),
        _parent{parent},
        _delegateLifecycle{delegateLifecycle},
        _delegateContext{delegateContext} {
    }
    std::shared_ptr<DelegatePlugin> shared_from_this() {
        return std::static_pointer_cast<DelegatePlugin>(AbstractPlugin::shared_from_this());
    }
    void lifecycle(Handle pluginRoot, Handle phase, const std::shared_ptr<Structish> & data) override;
};

//
// Native plugins are first-class, handled by the Nucleus itself
//
class NativePlugin : public AbstractPlugin {
private:
#if defined(USE_DLFCN)
    typedef void * nativeHandle_t;
    typedef uint32_t (*lifecycleFn_t)(uint32_t moduleHandle, uint32_t phase, uint32_t data);
#elif defined(USE_WINDLL)
    typedef HINSTANCE nativeHandle_t;
    typedef void (WINAPI *lifecycleFn_t)(uint32_t globalHandle, uint32_t phase, uint32_t data);
#endif
    volatile nativeHandle_t _handle { nullptr };
    volatile lifecycleFn_t _lifecycleFn { nullptr };

protected:
    std::shared_ptr<NativePlugin> shared_from_this() {
        return std::static_pointer_cast<NativePlugin>(AbstractPlugin::shared_from_this());
    }

public:
    explicit NativePlugin(Environment & environment, std::string_view name) :
        AbstractPlugin(environment, name) {
    }
    ~NativePlugin() override;
    void load(const std::string & filePath);
    void lifecycle(Handle pluginRoot, Handle phase, const std::shared_ptr<Structish> & data) override;
    bool isActive();
};

//
// Loader is responsible for handling all plugins
//
class PluginLoader : public AnchoredWithRoots {
private:
    std::vector<std::shared_ptr<Anchored>> getPlugins();

protected:
    std::shared_ptr<PluginLoader> shared_from_this() {
        return std::static_pointer_cast<PluginLoader>(AnchoredWithRoots::shared_from_this());
    }

public:
    explicit PluginLoader(Environment & environment) : AnchoredWithRoots(environment) {
    }

    void discoverPlugins();
    void discoverPlugin(const std::filesystem::directory_entry &entry);

    void loadNativePlugin(const std::string &name);

    void lifecycleBootstrap(const std::shared_ptr<Structish> & data);
    void lifecycleDiscover(const std::shared_ptr<Structish> & data);
    void lifecycleStart(const std::shared_ptr<Structish> & data);
    void lifecycleRun(const std::shared_ptr<Structish> & data);
    void lifecycleTerminate(const std::shared_ptr<Structish> & data);
    void lifecycle(Handle phase, const std::shared_ptr<Structish> & data);
};
