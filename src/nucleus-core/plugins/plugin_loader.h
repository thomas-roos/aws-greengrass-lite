#pragma once
#include "data/environment.h"
#include "data/handle_table.h"
#include "data/safe_handle.h"
#include "data/shared_struct.h"
#include <c_api.h>
#include <memory>
#include <string>
#include <list>
#include <shared_mutex>
#include <mutex>
#include <filesystem>

#if defined(USE_DLFCN)
#include <dlfcn.h>
#elif defined(USE_WINDLL)
#include <windows.h>
#endif

namespace plugins {
    //
    // Abstract plugins also acts as a global anchor for the given plugins module
    //
    class AbstractPlugin : public data::TrackingScope {
    protected:
        std::string _moduleName;
    public:
        explicit AbstractPlugin(data::Environment & environment, const std::string_view & name) :
                TrackingScope(environment), _moduleName{name} {
        }
        virtual void lifecycle(data::Handle pluginRoot, data::Handle phase, const std::shared_ptr<data::Structish> & data) = 0;
        virtual bool isActive() {
            return true;
        }
    };

    //
    // Delegate plugins is managed by a parent plugins
    //
    class DelegatePlugin : public AbstractPlugin {
    private:
        std::shared_ptr<AbstractPlugin> _parent; // delegate keeps parent in memory
        ggapiLifecycleCallback _delegateLifecycle { nullptr }; // called to handle this delegate
        uintptr_t _delegateContext {0}; // use of this defined by delegate
    public:
        explicit DelegatePlugin(data::Environment & environment, std::string_view name,
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
        void lifecycle(data::Handle pluginRoot, data::Handle phase, const std::shared_ptr<data::Structish> & data) override;
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
        explicit NativePlugin(data::Environment & environment, std::string_view name) :
            AbstractPlugin(environment, name) {
        }
        ~NativePlugin() override;
        void load(const std::string & filePath);
        void lifecycle(data::Handle pluginRoot, data::Handle phase, const std::shared_ptr<data::Structish> & data) override;
        bool isActive() override;
    };

    //
    // Loader is responsible for handling all plugins
    //
    class PluginLoader : public data::TrackingScope {
    private:
        std::vector<std::shared_ptr<data::ObjectAnchor>> getPlugins();

    protected:
        std::shared_ptr<PluginLoader> shared_from_this() {
            return std::static_pointer_cast<PluginLoader>(TrackingScope::shared_from_this());
        }

    public:
        explicit PluginLoader(data::Environment & environment) : TrackingScope(environment) {
        }

        void discoverPlugins();
        void discoverPlugin(const std::filesystem::directory_entry &entry);

        void loadNativePlugin(const std::string &name);

        void lifecycleBootstrap(const std::shared_ptr<data::Structish> & data);
        void lifecycleDiscover(const std::shared_ptr<data::Structish> & data);
        void lifecycleStart(const std::shared_ptr<data::Structish> & data);
        void lifecycleRun(const std::shared_ptr<data::Structish> & data);
        void lifecycleTerminate(const std::shared_ptr<data::Structish> & data);
        void lifecycle(data::Handle phase, const std::shared_ptr<data::Structish> & data);
    };
}

