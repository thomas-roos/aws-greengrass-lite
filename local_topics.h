#pragma once
#include <map>
#include <string>
#include <vector>
#include <functional>
#include "safe_handle.h"
#include "handle_table.h"

class AbstractCallback {
public:
    virtual ~AbstractCallback() = default;
    virtual Handle operator()(Handle taskHandle, Handle topicOrd, Handle dataStruct) = 0;
};

class Task;
class SubTask;
class TopicReceivers;
class SharedStruct;
class TopicReceiver : public AnchoredObject {
private:
    Handle _topicOrd;
    std::weak_ptr<TopicReceivers> _receivers;
    std::unique_ptr<AbstractCallback> _callback;

public:
    explicit TopicReceiver(Environment & environment, Handle topicOrd, TopicReceivers * receivers, std::unique_ptr<AbstractCallback> & callback);
    ~TopicReceiver() override;
    std::unique_ptr<SubTask> toSubTask(std::shared_ptr<Task> & task);
    std::shared_ptr<SharedStruct> runInTaskThread(std::shared_ptr<Task> & task, std::shared_ptr<SharedStruct> & dataIn);
};

class Task;
class LocalTopics;
class TopicReceivers : public std::enable_shared_from_this<TopicReceivers> {
private:
    Environment & _environment;
    Handle _topicOrd;
    std::weak_ptr<LocalTopics> _topics;
    std::vector<std::weak_ptr<TopicReceiver>> _receivers;
public:
    TopicReceivers(Environment &environment, Handle topicOrd, LocalTopics *topics);
    void cleanup();
    bool isEmpty() {
        return _receivers.empty();
    }
    std::shared_ptr<TopicReceiver> newReceiver(std::unique_ptr<AbstractCallback> & callback);
    void getCallOrder(std::vector<std::shared_ptr<TopicReceiver>> & callOrder);
};

class LocalTopics : public std::enable_shared_from_this<LocalTopics> {
private:
    Environment & _environment;
    std::map<Handle, std::shared_ptr<TopicReceivers>, Handle::CompLess> _topics;
public:
    explicit LocalTopics(Environment & environment) : _environment{environment} {
    }
    void cleanup();
    std::shared_ptr<TopicReceivers> testAndGetReceivers(Handle topicOrd);
    std::shared_ptr<TopicReceivers> getOrCreateReceivers(Handle topicOrd);
    std::shared_ptr<Anchored> subscribe(Handle anchor, Handle topicOrd, std::unique_ptr<AbstractCallback> & callback);
    static void applyCompletion(std::shared_ptr<Task> & task, Handle topicOrd, std::unique_ptr<AbstractCallback> & callback);
    void insertCallQueue(std::shared_ptr<Task> & task, Handle topicOrd);
};
