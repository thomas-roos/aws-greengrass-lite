#include "../data/globals.h"
#include <c_api.h>

uint32_t ggapiRegisterPlugin(uint32_t moduleHandle, uint32_t componentName, ggapiLifecycleCallback lifecycleCallback, uintptr_t callbackContext) {
    Global & global = Global::self();
    std::shared_ptr<AbstractPlugin> parentModule { global.environment.handleTable.getObject<AbstractPlugin>(Handle{moduleHandle}) };
    std::shared_ptr<DelegatePlugin> delegate {std::make_shared<DelegatePlugin>(
            global.environment,
            global.environment.stringTable.getString(Handle{componentName}),
            parentModule,
            lifecycleCallback,
            callbackContext)};
    std::shared_ptr<Anchored> anchor = global.loader->anchor(delegate.get()); // TODO: schedule bootstrap cycle
    return Handle{anchor}.asInt();
}
