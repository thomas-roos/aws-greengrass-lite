#include "data/globals.h"
#include <c_api.h>

uint32_t ggapiRegisterPlugin(uint32_t moduleHandle, uint32_t componentName, ggapiLifecycleCallback lifecycleCallback, uintptr_t callbackContext) {
    data::Global & global = data::Global::self();
    std::shared_ptr<plugins::AbstractPlugin> parentModule { global.environment.handleTable.getObject<plugins::AbstractPlugin>(data::Handle{moduleHandle}) };
    std::shared_ptr<plugins::DelegatePlugin> delegate {std::make_shared<plugins::DelegatePlugin>(
            global.environment,
            global.environment.stringTable.getString(data::Handle{componentName}),
            parentModule,
            lifecycleCallback,
            callbackContext)};
    std::shared_ptr<data::ObjectAnchor> anchor = global.loader->anchor(delegate.get()); // TODO: schedule bootstrap cycle
    return data::Handle{anchor}.asInt();
}
