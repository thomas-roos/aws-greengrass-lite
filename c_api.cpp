#include "environment.h"
#include "task.h"
#include "include/c_api.h"
#include "shared_struct.h"
#include "local_topics.h"

auto & g_environment { Environment::singleton()};
auto g_taskManager { std::make_shared<TaskManager>(g_environment) };
auto g_localTopics { std::make_unique<LocalTopics>(g_environment) };

uint32_t ggapiGetStringOrdinal(const char * bytes, size_t len) {
    return g_environment.stringTable.getOrCreateOrd(std::string_view {bytes, len}).asInt();
}

size_t ggapiGetOrdinalString(uint32_t ord, char * bytes, size_t len) {
    Handle ordH = Handle{ord};
    g_environment.stringTable.assertStringHandle(ordH);
    std::string s { g_environment.stringTable.getString(ordH) };
    CheckedBuffer checked(bytes, len);
    return checked.copy(s);
}

size_t ggapiGetOrdinalStringLen(uint32_t ord) {
    Handle ordH = Handle{ord};
    g_environment.stringTable.assertStringHandle(ordH);
    std::string s { g_environment.stringTable.getString(ordH) };
    return s.length();
}

uint32_t ggapiCreateTask(bool setThread) {
    std::shared_ptr<Anchored> taskAnchor {g_taskManager->createTask()};
    Handle taskHandle {taskAnchor};
    if (setThread) {
        taskAnchor->getObject<Task>()->getSetThreadSelf(taskHandle);
    }
    return taskHandle.asInt();
}

uint32_t ggapiGetCurrentTask(void) {
    return Task::getThreadSelf().asInt();
}

uint32_t ggapiCreateStruct(uint32_t anchorHandle) {
    auto ss {std::make_shared<SharedStruct>(g_environment)};
    auto owner {g_environment.handleTable.getObject<AnchoredWithRoots>(Handle{anchorHandle})};
    return owner->anchor(ss.get())->getHandle().asInt();
}
void ggapiStructPutInt32(uint32_t structHandle, uint32_t ord, uint32_t value) {
    auto ss {g_environment.handleTable.getObject<SharedStruct>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    StructElement newElement {static_cast<uint64_t>(value)};
    ss->put(ordH, newElement);
}

void ggapiStructPutInt64(uint32_t structHandle, uint32_t ord, uint64_t value) {
    auto ss {g_environment.handleTable.getObject<SharedStruct>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    StructElement newElement {value};
    ss->put(ordH, newElement);
}

void ggapiStructPutFloat32(uint32_t structHandle, uint32_t ord, float value) {
    auto ss {g_environment.handleTable.getObject<SharedStruct>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    StructElement newElement {value};
    ss->put(ordH, newElement);
}

void ggapiStructPutFloat64(uint32_t structHandle, uint32_t ord, double value) {
    auto ss {g_environment.handleTable.getObject<SharedStruct>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    StructElement newElement {value};
    ss->put(ordH, newElement);
}

void ggapiStructPutString(uint32_t structHandle, uint32_t ord, const char * bytes, size_t len) {
    auto ss {g_environment.handleTable.getObject<SharedStruct>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    StructElement newElement { std::string(bytes, len)};
    ss->put(ordH, newElement);
}

void ggapiStructPutStruct(uint32_t structHandle, uint32_t ord, uint32_t nestedHandle) {
    auto ss {g_environment.handleTable.getObject<SharedStruct>(Handle{structHandle})};
    auto s2 {g_environment.handleTable.getObject<SharedStruct>(Handle{nestedHandle})};
    Handle ordH = Handle{ord};
    StructElement newElement {s2};
    ss->put(ordH, newElement);
}

bool ggapiStructHasKey(uint32_t structHandle, uint32_t ord) {
    auto ss {g_environment.handleTable.getObject<SharedStruct>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    return ss->hasKey(ordH);
}

uint32_t ggapiStructGetInt32(uint32_t structHandle, uint32_t ord) {
    auto ss {g_environment.handleTable.getObject<SharedStruct>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    return static_cast<uint32_t >(ss->get(ordH));
}

uint64_t ggapiStructGetInt64(uint32_t structHandle, uint32_t ord) {
    auto ss {g_environment.handleTable.getObject<SharedStruct>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    return static_cast<uint64_t >(ss->get(ordH));
}

float ggapiStructGetFloat32(uint32_t structHandle, uint32_t ord) {
    auto ss {g_environment.handleTable.getObject<SharedStruct>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    return static_cast<float>(ss->get(ordH));
}

double ggapiStructGetFloat64(uint32_t structHandle, uint32_t ord) {
    auto ss {g_environment.handleTable.getObject<SharedStruct>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    return static_cast<double>(ss->get(ordH));
}

uint32_t ggapiStructGetStruct(uint32_t structHandle, uint32_t ord) {
    std::shared_ptr<Anchored> ss_anchor {g_environment.handleTable.getAnchor(Handle{structHandle})};
    std::shared_ptr<AnchoredWithRoots> ss_root { ss_anchor->getOwner()};
    auto ss {ss_anchor->getObject<SharedStruct>()};
    Handle ordH = Handle{ord};
    return ss_root->anchor(ss.get())->getHandle().asInt();
}

size_t ggapiStructGetStringLen(uint32_t structHandle, uint32_t ord) {
    auto ss {g_environment.handleTable.getObject<SharedStruct>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    std::string s = ss->get(ordH).getString();
    return s.length();
}

size_t ggapiStructGetString(uint32_t structHandle, uint32_t ord, char * buffer, size_t buflen) {
    auto ss {g_environment.handleTable.getObject<SharedStruct>(Handle{structHandle})};
    Handle ordH = Handle{ord};
    std::string s = ss->get(ordH).getString();
    CheckedBuffer checked(buffer, buflen);
    return checked.copy(s);
}

uint32_t ggapiAnchorHandle(uint32_t anchorHandle, uint32_t objectHandle) {
    auto ss {g_environment.handleTable.getObject<AnchoredObject>(Handle{objectHandle})};
    auto owner {g_environment.handleTable.getObject<AnchoredWithRoots>(Handle{anchorHandle})};
    return owner->anchor(ss.get())->getHandle().asInt();
}

void ggapiReleaseHandle(uint32_t objectHandle) {
    std::shared_ptr<Anchored> anchored {g_environment.handleTable.getAnchor(Handle{objectHandle})};
    anchored->release();
}

class NativeCallback : public AbstractCallback {
private:
    ggapiTopicCallback _callback;
public:
    explicit NativeCallback(ggapiTopicCallback callback) : _callback{callback} {
    }

    Handle operator()(Handle taskHandle, Handle topicOrd, Handle dataStruct) override {
        return Handle {_callback(taskHandle.asInt(), topicOrd.asInt(), dataStruct.asInt())};
    }
};

uint32_t ggapiSubscribeToTopic(uint32_t anchorHandle, uint32_t topicOrd, ggapiTopicCallback rxCallback) {
    std::unique_ptr<AbstractCallback> callback {new NativeCallback(rxCallback)};
    return g_localTopics->subscribe(Handle{anchorHandle}, Handle{topicOrd}, callback)->getHandle().asInt();
}

uint32_t ggapiSendToTopic(uint32_t topicOrd, uint32_t callStruct, time_t timeout) {
    Handle parentTask { Task::getThreadSelf() };
    std::shared_ptr<Task> parentTaskObj { g_environment.handleTable.getObject<Task>(parentTask) };
    std::shared_ptr<Anchored> taskAnchor {g_taskManager->createTask()}; // task is the anchor / return handle / context
    std::shared_ptr<Task> subTaskObj = taskAnchor->getObject<Task>();
    std::shared_ptr<SharedStruct> callDataStruct { g_environment.handleTable.getObject<SharedStruct>(Handle{callStruct}) };
    subTaskObj->setTimeout(g_environment.relativeToAbsoluteTime(timeout));
    g_localTopics->insertCallQueue(subTaskObj, Handle{topicOrd});
    subTaskObj->setData(callDataStruct);
    subTaskObj->runInThread();
    return Handle { parentTaskObj->anchor(subTaskObj->getData().get())}.asInt();
}

uint32_t ggapiWaitForTaskCompleted(uint32_t asyncTask, time_t timeout) {
    Handle parentTask { Task::getThreadSelf() };
    std::shared_ptr<Task> parentTaskObj { g_environment.handleTable.getObject<Task>(parentTask) };
    std::shared_ptr<Task> asyncTaskObj { g_environment.handleTable.getObject<Task>(Handle{asyncTask}) };
    if (asyncTaskObj->waitForCompletion(g_environment.relativeToAbsoluteTime(timeout))) {
        return Handle { parentTaskObj->anchor(asyncTaskObj->getData().get())}.asInt();
    } else {
        return 0;
    }
}

uint32_t ggapiSendToTopicAsync(uint32_t topicOrd, uint32_t callStruct, ggapiTopicCallback respCallback, time_t timeout) {
    std::shared_ptr<Anchored> taskAnchor {g_taskManager->createTask()}; // task is the anchor / return handle / context
    std::shared_ptr<Task> taskObject = taskAnchor->getObject<Task>();
    std::shared_ptr<SharedStruct> callDataStruct { g_environment.handleTable.getObject<SharedStruct>(Handle{callStruct}) };
    if (respCallback) {
        std::unique_ptr<AbstractCallback> callback{new NativeCallback(respCallback)};
        g_localTopics->applyCompletion(taskObject, Handle{topicOrd}, callback);
    }
    taskObject->setTimeout(g_environment.relativeToAbsoluteTime(timeout));
    g_localTopics->insertCallQueue(taskObject, Handle{topicOrd});
    taskObject->setData(callDataStruct);
    g_taskManager->queueAsyncTask(taskObject); // task must be ready, any thread can pick up once queued
    g_taskManager->allocateNextWorker(); // this ensures a worker picks it up
    return taskAnchor->getHandle().asInt();
}

uint32_t ggapiCallNext(uint32_t dataStruct) {
    Handle taskHandle { Task::getThreadSelf() };
    if (!taskHandle) {
        return 0;
    }
    auto taskObj { g_environment.handleTable.getObject<Task>(taskHandle)};
    if (dataStruct) {
        auto dataObj {g_environment.handleTable.getObject<SharedStruct>(Handle{dataStruct})};
        taskObj->setData(dataObj);
    }
    std::shared_ptr<SharedStruct> dataIn { taskObj->getData() };
    std::shared_ptr<SharedStruct> dataOut { taskObj->runInThreadCallNext(taskObj, dataIn) };
    return Handle{taskObj->anchor(dataOut.get())}.asInt();
}
