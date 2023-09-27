#include <cpp_api.h>
#include <thread>
#include <iostream>

struct Keys {
    ggapi::StringOrd start { "start"};
    ggapi::StringOrd run { "run"};
    ggapi::StringOrd publishToIoTCoreTopic {"aws.greengrass.PublishToIoTCore"};
    ggapi::StringOrd topicName {"topicName"};
    ggapi::StringOrd qos {"qos"};
    ggapi::StringOrd payload {"payload"};
    ggapi::StringOrd retain {"retain"};
    ggapi::StringOrd userProperties {"userProperties"};
    ggapi::StringOrd messageExpiryIntervalSeconds {"messageExpiryIntervalSeconds"};
    ggapi::StringOrd correlationData {"correlationData"};
    ggapi::StringOrd responseTopic {"responseTopic"};
    ggapi::StringOrd payloadFormat {"payloadFormat"};
    ggapi::StringOrd contentType {"contentType"};
};
const Keys keys;
std::thread asyncThread;

void asyncThreadFn();

extern "C" [[maybe_unused]] EXPORT void greengrass_lifecycle(uint32_t moduleHandle, uint32_t phase, uint32_t data) {
    std::cout << "Running lifecycle plugins 2... " << ggapi::StringOrd{phase}.toString() << std::endl;
    ggapi::StringOrd phaseOrd{phase};
    if (phaseOrd == keys.run) {
        asyncThread = std::thread{asyncThreadFn};
        asyncThread.detach();
    }
}

ggapi::Struct publishToIoTCoreListener(ggapi::ObjHandle task, ggapi::StringOrd topic, ggapi::Struct callData) {
    // real work
    std::string destTopic { callData.getString(keys.topicName)};
    int qos { static_cast<int>(callData.getInt32(keys.qos))};
    ggapi::Struct payload {callData.getStruct(keys.payload)};
    // ...
    // construct response
    ggapi::Struct response = task.createStruct();
    response.put("status", 1U);
    // return response
    return response;
}

ggapi::Struct publishToIoTCoreResponder(ggapi::ObjHandle task, ggapi::StringOrd topic, ggapi::Struct respData) {
    if (!respData) {
        // unhandled
        return respData;
    }
    uint32_t status { respData.getInt32("status") };
    return respData;
}

void asyncThreadFn() {
    std::cout << "Running async plugins 2..." << std::endl;
    auto threadTask = ggapi::ObjHandle::claimThread(); // assume long-running thread, this provides a long-running task handle

    ggapi::ObjHandle publishToIoTCoreListenerHandle {threadTask.subscribeToTopic(keys.publishToIoTCoreTopic, publishToIoTCoreListener)};

    auto request {threadTask.createStruct() };
    request.put(keys.topicName, "some-cloud-topic")
            .put(keys.qos, "1") // string gets converted to int later
            .put(keys.payload, threadTask.createStruct().put("Foo", 1U));

    // Async style
    ggapi::ObjHandle newTask = threadTask.sendToTopicAsync(keys.publishToIoTCoreTopic, request, publishToIoTCoreResponder, -1);
    ggapi::Struct respData = newTask.waitForTaskCompleted();
    uint32_t status { respData.getInt32("status") };

    // Sync style
    ggapi::Struct syncRespData = threadTask.sendToTopic(keys.publishToIoTCoreTopic, request);
    uint32_t syncStatus { syncRespData.getInt32("status") };

    std::cout << "Ping..." << std::endl;

    ggapi::Struct pingData = threadTask.createStruct().put("ping", "abcde");
    ggapi::Struct pongData = threadTask.sendToTopic(ggapi::StringOrd{"test"}, pingData);
    std::string pongString = pongData.getString("pong");

    std::cout << "Pong..." << pongString << std::endl;

    ggapi::ObjHandle::releaseThread();
}
