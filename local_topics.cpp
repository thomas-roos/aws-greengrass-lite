#include "local_topics.h"
#include "environment.h"
#include <shared_mutex>
#include "task.h"
#include "shared_struct.h"

TopicReceiver::~TopicReceiver() {
    if (!_receivers.expired()) {
        std::shared_ptr<TopicReceivers> receivers{_receivers};
        receivers->cleanup();
    }
}

TopicReceiver::TopicReceiver(Environment &environment, Handle topicOrd, TopicReceivers * receivers, std::unique_ptr<AbstractCallback> &callback) :
    AnchoredObject{environment},
    _topicOrd{topicOrd},
    _receivers{receivers->weak_from_this()},
    _callback{std::move(callback)} {

}

TopicReceivers::TopicReceivers(Environment &environment, Handle topicOrd, LocalTopics *topics) :
    _environment{environment}, _topicOrd{topicOrd}, _topics {topics->weak_from_this()} {
}

void TopicReceivers::cleanup() {
    // scoped lock
    {
        std::unique_lock guard{_environment.sharedLocalTopicsMutex};
        (void)std::remove_if(_receivers.begin(), _receivers.end(), [&](const auto &item) {
            return item.expired();
        });
    }
    // lock must be released before this step
    if (_receivers.empty() && !_topics.expired()) {
        std::shared_ptr<LocalTopics> topics{_topics};
        topics->cleanup();
    }
}

void LocalTopics::cleanup() {
    std::unique_lock guard{_environment.sharedLocalTopicsMutex};
    for (auto i = _topics.begin(); i != _topics.end(); ++i) {
        if (i->second->isEmpty()) {
            _topics.erase(i);
        }
    }
}

std::shared_ptr<TopicReceiver> TopicReceivers::newReceiver(std::unique_ptr<AbstractCallback> &callback) {
    std::shared_ptr<TopicReceiver> receiver {std::make_shared<TopicReceiver>(_environment, _topicOrd, this, callback)};
    std::unique_lock guard{_environment.sharedLocalTopicsMutex};
    _receivers.push_back(receiver);
    return receiver;
}

std::shared_ptr<TopicReceivers> LocalTopics::testAndGetReceivers(Handle topicOrd) {
    std::shared_lock guard{_environment.sharedLocalTopicsMutex};
    auto i = _topics.find(topicOrd);
    if (i == _topics.end()) {
        return {};
    } else {
        return i->second;
    }
}

std::shared_ptr<TopicReceivers> LocalTopics::getOrCreateReceivers(Handle topicOrd) {
    std::shared_ptr<TopicReceivers> receivers = testAndGetReceivers(topicOrd);
    if (receivers) {
        return receivers;
    }
    std::unique_lock guard{_environment.sharedLocalTopicsMutex};
    auto i = _topics.find(topicOrd);
    if (i != _topics.end()) {
        // rare edge case
        return i->second;
    }
    receivers = std::make_shared<TopicReceivers>(_environment, topicOrd, this);
    _topics[topicOrd] = receivers;
    return receivers;
}

std::shared_ptr<Anchored> LocalTopics::subscribe(Handle anchor, Handle topicOrd, std::unique_ptr<AbstractCallback> & callback) {
    auto root = _environment.handleTable.getObject<AnchoredWithRoots>(anchor);
    std::shared_ptr<TopicReceivers> receivers = getOrCreateReceivers(topicOrd);
    std::shared_ptr<TopicReceiver> receiver = receivers->newReceiver(callback);
    return root->anchor(receiver.get()); // if handle or root goes away, unsubscribe
}

void LocalTopics::insertCallQueue(std::shared_ptr<Task> & task, Handle topicOrd) {
    std::shared_ptr<TopicReceivers> receivers = testAndGetReceivers(topicOrd);
    if (receivers == nullptr || receivers->isEmpty()) {
        return;
    }
    std::vector<std::shared_ptr<TopicReceiver>> callOrder;
    receivers->getCallOrder(callOrder);
    for (const auto& i : callOrder) {
        std::unique_ptr<SubTask> subtask {i->toSubTask(task)};
        task->addSubtask(subtask);
    }
}

void TopicReceivers::getCallOrder(std::vector<std::shared_ptr<TopicReceiver>> & callOrder) {
    if (isEmpty()) {
        return;
    }
    std::shared_lock guard{_environment.sharedLocalTopicsMutex};
    for (auto ri = _receivers.rbegin(); ri != _receivers.rend(); ++ri) {
        if (!ri->expired()) {
            callOrder.push_back(ri->lock());
        }
    }
}

class ReceiverSubTask : public SubTask
{
private:
    std::shared_ptr<TopicReceiver> _receiver;
public:
    explicit ReceiverSubTask(std::shared_ptr<TopicReceiver> & receiver) :
        _receiver{receiver} {
    }
    std::shared_ptr<SharedStruct> runInThread(std::shared_ptr<Task> & task, std::shared_ptr<SharedStruct> & dataIn) override;
};

std::unique_ptr<SubTask> TopicReceiver::toSubTask(std::shared_ptr<Task> & task) {
    std::shared_lock guard{_environment.sharedLocalTopicsMutex};
    std::shared_ptr<TopicReceiver> receiver {std::static_pointer_cast<TopicReceiver>(shared_from_this())};
    std::unique_ptr<SubTask> subTask {new ReceiverSubTask(receiver) };
    return subTask;
}

std::shared_ptr<SharedStruct> ReceiverSubTask::runInThread(std::shared_ptr<Task> & task, std::shared_ptr<SharedStruct> & dataIn) {
    return _receiver->runInTaskThread(task, dataIn);
}

std::shared_ptr<SharedStruct> TopicReceiver::runInTaskThread(std::shared_ptr<Task> & task, std::shared_ptr<SharedStruct> & dataIn) {
    Handle dataHandle { task->anchor(dataIn.get()) };
    Handle resp = _callback->operator()(task->getSelf(), _topicOrd, dataHandle);
    std::shared_ptr<SharedStruct> respData;
    if (resp) {
        respData = _environment.handleTable.getObject<SharedStruct>(resp);
    }
    return respData;
}

class CompletionSubTask : public SubTask
{
private:
    Handle _topicOrd;
    std::unique_ptr<AbstractCallback> _callback;
public:
    explicit CompletionSubTask(Handle topicOrd, std::unique_ptr<AbstractCallback> &callback) :
            _topicOrd{topicOrd},
            _callback{std::move(callback)} {
    }
    std::shared_ptr<SharedStruct> runInThread(std::shared_ptr<Task> & task, std::shared_ptr<SharedStruct> & result) override;
};

std::shared_ptr<SharedStruct> CompletionSubTask::runInThread(std::shared_ptr<Task> & task, std::shared_ptr<SharedStruct> & result) {
    Handle dataHandle { task->anchor(result.get()) };
    (void)_callback->operator()(task->getSelf(), _topicOrd, dataHandle);
    return nullptr;
}

void LocalTopics::applyCompletion(std::shared_ptr<Task> &task, Handle topicOrd,
                                  std::unique_ptr<AbstractCallback> &callback) {
    if (!callback) {
        return;
    }
    std::unique_ptr<SubTask> subTask {new CompletionSubTask(topicOrd, callback) };
    task->setCompletion(subTask);
}
