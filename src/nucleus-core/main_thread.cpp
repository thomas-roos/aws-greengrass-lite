#include <cpp_api.h>
#include "shared_struct.h"
#include "globals.h"

// Main blocking thread, called by containing process

int ggapiMainThread() {
    Global & global = Global::self();
    auto threadTask = ggapi::ObjHandle::claimThread(); // assume long-running thread, this provides a long-running task handle

    // This needs to be subsumed by the lifecyle management - not yet implemented, so current approach is hacky
    global.loader->discoverPlugins();
    std::shared_ptr<SharedStruct> emptyStruct {std::make_shared<SharedStruct>(global.environment)}; // TODO, empty for now
    global.loader->lifecycleBootstrap(emptyStruct);
    global.loader->lifecycleDiscover(emptyStruct);
    global.loader->lifecycleStart(emptyStruct);
    global.loader->lifecycleRun(emptyStruct);

    (void)threadTask.waitForTaskCompleted(); // essentially blocks forever but allows main thread to do work
    // TODO: This is currently crashing!
    global.loader->lifecycleTerminate(emptyStruct);
    return 0; // currently unreachable
}
