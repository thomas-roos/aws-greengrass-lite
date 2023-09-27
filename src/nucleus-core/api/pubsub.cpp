#include "data/globals.h"
#include <c_api.h>

class NativeCallback : public pubsub::AbstractCallback {
private:
    ggapiTopicCallback _callback;
    uintptr_t _context;
public:
    explicit NativeCallback(ggapiTopicCallback callback, uintptr_t context) : _callback{callback}, _context{context} {
    }

    data::Handle operator()(data::Handle taskHandle, data::Handle topicOrd, data::Handle dataStruct) override {
        return data::Handle {_callback(_context, taskHandle.asInt(), topicOrd.asInt(), dataStruct.asInt())};
    }
};

uint32_t ggapiSubscribeToTopic(uint32_t anchorHandle, uint32_t topicOrd, ggapiTopicCallback rxCallback, uintptr_t context) {
    data::Global & global = data::Global::self();
    std::unique_ptr<pubsub::AbstractCallback> callback {new NativeCallback(rxCallback, context)};
    return global.lpcTopics->subscribe(data::Handle{anchorHandle}, data::Handle{topicOrd}, callback)->getHandle().asInt();
}

uint32_t ggapiSendToTopic(uint32_t topicOrd, uint32_t callStruct, int32_t timeout) {
    data::Global & global = data::Global::self();
    data::Handle parentTask {tasks::Task::getThreadSelf() };
    std::shared_ptr<tasks::Task> parentTaskObj {global.environment.handleTable.getObject<tasks::Task>(parentTask) };
    std::shared_ptr<data::ObjectAnchor> taskAnchor {global.taskManager->createTask()}; // task is the anchor / return handle / context
    std::shared_ptr<tasks::Task> subTaskObj = taskAnchor->getObject<tasks::Task>();
    std::shared_ptr<data::Structish> callDataStruct {global.environment.handleTable.getObject<data::Structish>(
            data::Handle{callStruct}) };
    ExpireTime expireTime = global.environment.translateExpires(timeout);
    subTaskObj->setTimeout(expireTime);
    global.lpcTopics->insertCallQueue(subTaskObj, data::Handle{topicOrd});
    subTaskObj->setData(callDataStruct);
    global.taskManager->queueTask(subTaskObj); // task must be ready, any thread can pick up once queued
    // note, we don't allocate next worker, preferring to task-steal, but ok if an idle worker picks it up
    if (subTaskObj->waitForCompletion(expireTime)) {
        return data::Handle {parentTaskObj->anchor(subTaskObj->getData().get())}.asInt();
    } else {
        return 0;
    }
}

uint32_t ggapiSendToTopicAsync(uint32_t topicOrd, uint32_t callStruct, ggapiTopicCallback respCallback, uintptr_t context, int32_t timeout) {
    data::Global & global = data::Global::self();
    std::shared_ptr<data::ObjectAnchor> taskAnchor {global.taskManager->createTask()}; // task is the anchor / return handle / context
    std::shared_ptr<tasks::Task> taskObject = taskAnchor->getObject<tasks::Task>();
    std::shared_ptr<data::Structish> callDataStruct {global.environment.handleTable.getObject<data::Structish>(
            data::Handle{callStruct}) };
    if (respCallback) {
        std::unique_ptr<pubsub::AbstractCallback> callback{new NativeCallback(respCallback, context)};
        global.lpcTopics->applyCompletion(taskObject, data::Handle{topicOrd}, callback);
    }
    ExpireTime expireTime = global.environment.translateExpires(timeout);
    taskObject->setTimeout(expireTime);
    global.lpcTopics->insertCallQueue(taskObject, data::Handle{topicOrd});
    taskObject->setData(callDataStruct);
    global.taskManager->queueTask(taskObject); // task must be ready, any thread can pick up once queued
    global.taskManager->allocateNextWorker(); // this ensures a worker picks it up
    return taskAnchor->getHandle().asInt();
}
//
//uint32_t ggapiCallNext(uint32_t dataStruct) {
//    Handle taskHandle { Task::getThreadSelf() };
//    if (!taskHandle) {
//        throw std::runtime_error("Expected thread to be associated with a task");
//    }
//    auto taskObj { global.environment.handleTable.getObject<Task>(taskHandle)};
//    if (dataStruct) {
//        auto dataObj {global.environment.handleTable.getObject<Structish>(Handle{dataStruct})};
//        taskObj->setData(dataObj);
//    }
//    // task is permitted to run everything up to but not including finalization
//    // and transition control back to this function
//    std::shared_ptr<Structish> dataIn { taskObj->getData() };
//    std::shared_ptr<Structish> dataOut { taskObj->runInThreadCallNext(taskObj, dataIn) };
//    return Handle{taskObj->anchor(dataOut.get())}.asInt();
//}
