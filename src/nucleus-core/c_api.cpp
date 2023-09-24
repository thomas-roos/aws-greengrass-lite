#include "environment.h"
#include "task.h"
#include "shared_struct.h"
#include "local_topics.h"
#include "globals.h"
#include "expire_time.h"
#include <c_api.h>

uint32_t ggapiGetStringOrdinal(const char * bytes, size_t len) {
    Global & global = Global::self();
    return global.environment.stringTable.getOrCreateOrd(std::string {bytes, len}).asInt();
}

size_t ggapiGetOrdinalString(uint32_t ord, char * bytes, size_t len) {
    Global & global = Global::self();
    Handle ordH = Handle{ord};
    global.environment.stringTable.assertStringHandle(ordH);
    std::string s { global.environment.stringTable.getString(ordH) };
    CheckedBuffer checked(bytes, len);
    return checked.copy(s);
}

size_t ggapiGetOrdinalStringLen(uint32_t ord) {
    Global & global = Global::self();
    Handle ordH = Handle{ord};
    global.environment.stringTable.assertStringHandle(ordH);
    std::string s { global.environment.stringTable.getString(ordH) };
    return s.length();
}

uint32_t ggapiClaimThread() {
    Global & global = Global::self();
    std::shared_ptr<FixedTaskThread> thread {std::make_shared<FixedTaskThread>(global.environment, global.taskManager)};
    return Handle{thread->claimFixedThread()}.asInt();
}

void ggapiReleaseThread() {
    Global & global = Global::self();
    std::shared_ptr<TaskThread> thread = FixedTaskThread::getThreadContext();
    thread->releaseFixedThread();
}

uint32_t ggapiGetCurrentTask(void) {
    return Task::getThreadSelf().asInt();
}

uint32_t ggapiCreateStruct(uint32_t anchorHandle) {
    Global & global = Global::self();
    if (anchorHandle == 0) {
        anchorHandle = Task::getThreadSelf().asInt();
    }
    auto ss {std::make_shared<SharedStruct>(global.environment)};
    auto owner {global.environment.handleTable.getObject<AnchoredWithRoots>(Handle{anchorHandle})};
    return owner->anchor(ss.get())->getHandle().asInt();
}
void ggapiStructPutInt32(uint32_t structHandle, uint32_t ord, uint32_t value) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    StructElement newElement {static_cast<uint64_t>(value)};
    ss->put(ordH, newElement);
}

void ggapiStructPutInt64(uint32_t structHandle, uint32_t ord, uint64_t value) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    StructElement newElement {value};
    ss->put(ordH, newElement);
}

void ggapiStructPutFloat32(uint32_t structHandle, uint32_t ord, float value) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    StructElement newElement {value};
    ss->put(ordH, newElement);
}

void ggapiStructPutFloat64(uint32_t structHandle, uint32_t ord, double value) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    StructElement newElement {value};
    ss->put(ordH, newElement);
}

void ggapiStructPutString(uint32_t structHandle, uint32_t ord, const char * bytes, size_t len) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    StructElement newElement { std::string(bytes, len)};
    ss->put(ordH, newElement);
}

void ggapiStructPutStruct(uint32_t structHandle, uint32_t ord, uint32_t nestedHandle) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    auto s2 {global.environment.handleTable.getObject<Structish>(Handle{nestedHandle})};
    Handle ordH = Handle{ord};
    StructElement newElement {s2};
    ss->put(ordH, newElement);
}

bool ggapiStructHasKey(uint32_t structHandle, uint32_t ord) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    return ss->hasKey(ordH);
}

uint32_t ggapiStructGetInt32(uint32_t structHandle, uint32_t ord) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    return static_cast<uint32_t >(ss->get(ordH));
}

uint64_t ggapiStructGetInt64(uint32_t structHandle, uint32_t ord) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    return static_cast<uint64_t >(ss->get(ordH));
}

float ggapiStructGetFloat32(uint32_t structHandle, uint32_t ord) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    return static_cast<float>(ss->get(ordH));
}

double ggapiStructGetFloat64(uint32_t structHandle, uint32_t ord) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    return static_cast<double>(ss->get(ordH));
}

uint32_t ggapiStructGetStruct(uint32_t structHandle, uint32_t ord) {
    Global & global = Global::self();
    std::shared_ptr<Anchored> ss_anchor {global.environment.handleTable.getAnchor(Handle{structHandle})};
    std::shared_ptr<AnchoredWithRoots> ss_root { ss_anchor->getOwner()};
    auto ss {ss_anchor->getObject<Structish>()};
    Handle ordH = Handle{ord};
    return ss_root->anchor(ss.get())->getHandle().asInt();
}

size_t ggapiStructGetStringLen(uint32_t structHandle, uint32_t ord) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    std::string s = ss->get(ordH).getString();
    return s.length();
}

size_t ggapiStructGetString(uint32_t structHandle, uint32_t ord, char * buffer, size_t buflen) {
    Global & global = Global::self();
    auto ss {global.environment.handleTable.getObject<Structish>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    std::string s = ss->get(ordH).getString();
    CheckedBuffer checked(buffer, buflen);
    return checked.copy(s);
}

uint32_t ggapiAnchorHandle(uint32_t anchorHandle, uint32_t objectHandle) {
    Global & global = Global::self();
    if (anchorHandle == 0) {
        anchorHandle = Task::getThreadSelf().asInt();
    }
    auto ss {global.environment.handleTable.getObject<AnchoredObject>(Handle{objectHandle})};
    auto owner {global.environment.handleTable.getObject<AnchoredWithRoots>(Handle{anchorHandle})};
    return owner->anchor(ss.get())->getHandle().asInt();
}

void ggapiReleaseHandle(uint32_t objectHandle) {
    Global & global = Global::self();
    std::shared_ptr<Anchored> anchored {global.environment.handleTable.getAnchor(Handle{objectHandle})};
    // releasing a non-existing handle is a no-op as it may have been garbage collected
    if (anchored) {
        anchored->release();
    }
}

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
