#include "../include/cpp_api.h"

struct Keys {
    const ggapi::StringOrd start { "start"};
    const ggapi::StringOrd run { "run"};
    const ggapi::StringOrd publishToIoTCoreTopic {"aws.greengrass.PublishToIoTCore"};
    const ggapi::StringOrd topicName {"topicName"};
    const ggapi::StringOrd qos {"qos"};
    const ggapi::StringOrd payload {"payload"};
    const ggapi::StringOrd retain {"retain"};
    const ggapi::StringOrd userProperties {"userProperties"};
    const ggapi::StringOrd messageExpiryIntervalSeconds {"messageExpiryIntervalSeconds"};
    const ggapi::StringOrd correlationData {"correlationData"};
    const ggapi::StringOrd responseTopic {"responseTopic"};
    const ggapi::StringOrd payloadFormat {"payloadFormat"};
    const ggapi::StringOrd contentType {"contentType"};

    static const Keys & get() {
        static std::unique_ptr<Keys> keyRef;
        if (keyRef == nullptr) {
            keyRef = std::make_unique<Keys>();
        }
        return *keyRef;
    }
};


uint32_t testListener(uint32_t taskId, uint32_t topicOrdId, uint32_t dataId) {
    ggapi::ObjHandle task {taskId};
    ggapi::StringOrd topic {topicOrdId};
    ggapi::Struct callData {dataId};
    std::string pingMessage { callData.getString("ping")};
    ggapi::Struct response = task.createStruct();
    response.put("pong", pingMessage);
    return response.getHandleId();
}


void doStartPhase() {
    (void)ggapi::ObjHandle::thisTask().subscribeToTopic(ggapi::StringOrd{"test"}, testListener);
}

void doRunPhase() {

}

extern "C" void greengrass_initialize() {
}

extern "C" void greengrass_lifecycle(uint32_t phase) {
    const auto & keys = Keys::get();

    ggapi::StringOrd phaseOrd{phase};
    if (phaseOrd == keys.start) {
        doStartPhase();
    } else if (phaseOrd == keys.run) {
        doRunPhase();
    }
}
