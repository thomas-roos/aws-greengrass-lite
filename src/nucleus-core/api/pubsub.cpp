#include "../data/globals.h"
#include <c_api.h>

class NativeCallback : public AbstractCallback {
private:
    ggapiTopicCallback _callback;
    uintptr_t _context;
public:
    explicit NativeCallback(ggapiTopicCallback callback, uintptr_t context) : _callback{callback}, _context{context} {
    }

    Handle operator()(Handle taskHandle, Handle topicOrd, Handle dataStruct) override {
        return Handle {_callback(_context, taskHandle.asInt(), topicOrd.asInt(), dataStruct.asInt())};
    }
};

uint32_t ggapiSubscribeToTopic(uint32_t anchorHandle, uint32_t topicOrd, ggapiTopicCallback rxCallback, uintptr_t context) {
    Global & global = Global::self();
    std::unique_ptr<AbstractCallback> callback {new NativeCallback(rxCallback, context)};
    return global.lpcTopics->subscribe(Handle{anchorHandle}, Handle{topicOrd}, callback)->getHandle().asInt();
}

uint32_t ggapiSendToTopic(uint32_t topicOrd, uint32_t callStruct, int32_t timeout) {
    Global & global = Global::self();
    Handle parentTask { Task::getThreadSelf() };
    std::shared_ptr<Task> parentTaskObj { global.environment.handleTable.getObject<Task>(parentTask) };
    std::shared_ptr<Anchored> taskAnchor {global.taskManager->createTask()}; // task is the anchor / return handle / context
    std::shared_ptr<Task> subTaskObj = taskAnchor->getObject<Task>();
    std::shared_ptr<Structish> callDataStruct {global.environment.handleTable.getObject<Structish>(Handle{callStruct}) };
    ExpireTime expireTime = global.environment.translateExpires(timeout);
    subTaskObj->setTimeout(expireTime);
    global.lpcTopics->insertCallQueue(subTaskObj, Handle{topicOrd});
    subTaskObj->setData(callDataStruct);
    global.taskManager->queueTask(subTaskObj); // task must be ready, any thread can pick up once queued
    // note, we don't allocate next worker, preferring to task-steal, but ok if an idle worker picks it up
    if (subTaskObj->waitForCompletion(expireTime)) {
        return Handle { parentTaskObj->anchor(subTaskObj->getData().get())}.asInt();
    } else {
        return 0;
    }
}

uint32_t ggapiWaitForTaskCompleted(uint32_t asyncTask, int32_t timeout) {
    Global & global = Global::self();
    Handle parentTask { Task::getThreadSelf() };
    std::shared_ptr<Task> parentTaskObj { global.environment.handleTable.getObject<Task>(parentTask) };
    std::shared_ptr<Task> asyncTaskObj { global.environment.handleTable.getObject<Task>(Handle{asyncTask}) };
    ExpireTime expireTime = global.environment.translateExpires(timeout);
    if (asyncTaskObj->waitForCompletion(expireTime)) {
        return Handle { parentTaskObj->anchor(asyncTaskObj->getData().get())}.asInt();
    } else {
        return 0;
    }
}

uint32_t ggapiSendToTopicAsync(uint32_t topicOrd, uint32_t callStruct, ggapiTopicCallback respCallback, uintptr_t context, int32_t timeout) {
    Global & global = Global::self();
    std::shared_ptr<Anchored> taskAnchor {global.taskManager->createTask()}; // task is the anchor / return handle / context
    std::shared_ptr<Task> taskObject = taskAnchor->getObject<Task>();
    std::shared_ptr<Structish> callDataStruct {global.environment.handleTable.getObject<Structish>(Handle{callStruct}) };
    if (respCallback) {
        std::unique_ptr<AbstractCallback> callback{new NativeCallback(respCallback, context)};
        global.lpcTopics->applyCompletion(taskObject, Handle{topicOrd}, callback);
    }
    ExpireTime expireTime = global.environment.translateExpires(timeout);
    taskObject->setTimeout(expireTime);
    global.lpcTopics->insertCallQueue(taskObject, Handle{topicOrd});
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
