#include <iostream>
#include <cpp_api.h>
//#include "nucleus-core/plugin_loader.h"

struct Keys {
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

uint32_t publishToIoTCoreListener(uint32_t taskId, uint32_t topicOrdId, uint32_t dataId) {
    ggapi::ObjHandle task {taskId};
    ggapi::StringOrd topic {topicOrdId};
    ggapi::Struct callData {dataId};
    // ordinal constants not needed, but this is how to do it
    const Keys & keys {Keys::get()};
    // real work
    std::string destTopic { callData.getString(keys.topicName)};
    int qos { (int)callData.getInt32(keys.qos)};
    ggapi::Struct payload {callData.getStruct(keys.payload)};
    // ...
    // construct response
    ggapi::Struct response = task.createStruct();
    response.put("status", 1U);
    // return response
    return response.getHandleId();
}

uint32_t publishToIoTCoreResponder(uint32_t taskId, uint32_t topicOrdId, uint32_t dataId) {
    if (dataId == 0) {
        // unhandled
        return 0;
    }
    ggapi::ObjHandle task {taskId};
    ggapi::StringOrd topic {topicOrdId};
    ggapi::Struct respData {dataId};
    uint32_t status { respData.getInt32("status") };
    return 0;
}

int main() {
    std::cout << "Running..." << std::endl;
    auto threadTask = ggapi::ObjHandle::claimThread(); // assume long-running thread, this provides a long-running task handle

    const Keys & keys {Keys::get()};
    ggapi::ObjHandle publishToIoTCoreListenerHandle {threadTask.subscribeToTopic(keys.publishToIoTCoreTopic, publishToIoTCoreListener)};

    auto request {threadTask.createStruct() };
    request.put(keys.topicName, "some-cloud-topic")
        .put(keys.qos, "1") // string gets converted to int later
        .put(keys.payload, threadTask.createStruct().put("Foo", 1U));

    // Async style
    ggapi::ObjHandle newTask = threadTask.sendToTopicAsync(keys.publishToIoTCoreTopic, request, publishToIoTCoreResponder);
    ggapi::Struct respData = newTask.waitForTaskCompleted();
    uint32_t status { respData.getInt32("status") };

    // Sync style
    ggapi::Struct syncRespData = threadTask.sendToTopic(keys.publishToIoTCoreTopic, request);
    uint32_t syncStatus { syncRespData.getInt32("status") };

//    PluginLoader loader;
//    loader.discoverPlugins();
//    loader.initialize();
//    loader.lifecycleStart();
//    loader.lifecycleRun();
//
//    std::cout << "Ping..." << std::endl;
//
//    ggapi::Struct pingData = threadTask.createStruct().put("ping", "abcde");
//    ggapi::Struct pongData = threadTask.sendToTopic(ggapi::StringOrd{"test"}, pingData);
//    std::string pongString = pongData.getString("pong");
//
//    std::cout << "Pong..." << pongString << std::endl;

    return 0;
}
