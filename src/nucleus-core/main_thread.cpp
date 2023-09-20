#include <cpp_api.h>
#include "globals.h"

Environment g_environment;
std::shared_ptr<TaskManager> g_taskManager { std::make_shared<TaskManager>(g_environment) };
std::shared_ptr<LocalTopics> g_localTopics { std::make_unique<LocalTopics>(g_environment) };
PluginLoader g_loader { g_environment };

// Main blocking thread, called by containing process

int ggapiMainThread() {
    auto threadTask = ggapi::ObjHandle::claimThread(); // assume long-running thread, this provides a long-running task handle

    // This needs to be subsumed by the lifecyle management - not yet implemented, so current approach is hacky
    g_loader.discoverPlugins();
    g_loader.initialize();
    g_loader.lifecycleStart();
    g_loader.lifecycleRun();

    (void)threadTask.waitForTaskCompleted(); // essentially blocks forever but allows main thread to do work
    return 0; // currently unreachable
}
