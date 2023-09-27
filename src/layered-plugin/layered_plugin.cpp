#include <cpp_api.h>
#include <iostream>

// A layered plugins is permitted to add additional abstract plugins

const ggapi::StringOrd DISCOVER_PHASE {"discover"};

void doDiscoverPhase(ggapi::ObjHandle moduleHandle, ggapi::Struct phaseData);

extern "C" [[maybe_unused]] EXPORT void greengrass_lifecycle(uint32_t moduleHandle, uint32_t phase, uint32_t data) {
    std::cout << "Running layered lifecycle plugins... " << ggapi::StringOrd{phase}.toString() << std::endl;
    ggapi::StringOrd phaseOrd{phase};
    if (phaseOrd == DISCOVER_PHASE) {
        doDiscoverPhase(ggapi::ObjHandle{moduleHandle}, ggapi::Struct{data});
    }
}

void greengrass_delegate_lifecycle(ggapi::ObjHandle moduleHandle, ggapi::StringOrd phase, ggapi::Struct data) {
    std::cout << "Running lifecycle delegate... " << moduleHandle.getHandleId() << " phase " << phase.toString() << std::endl;
}

void doDiscoverPhase(ggapi::ObjHandle moduleHandle, ggapi::Struct phaseData) {
    ggapi::ObjHandle nestedPlugin = moduleHandle.registerPlugin(ggapi::StringOrd{"MyDelegate"}, greengrass_delegate_lifecycle);
}
