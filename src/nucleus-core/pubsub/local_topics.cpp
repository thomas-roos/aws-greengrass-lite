#include "local_topics.h"
#include "tasks/task.h"
#include <shared_mutex>

pubsub::TopicReceiver::~TopicReceiver() {
    if (!_receivers.expired()) {
        std::shared_ptr<pubsub::TopicReceivers> receivers{_receivers};
        receivers->cleanup();
    }
}

pubsub::TopicReceiver::TopicReceiver(data::Environment &environment, data::Handle topicOrd, pubsub::TopicReceivers * receivers, std::unique_ptr<pubsub::AbstractCallback> &callback) :
    data::TrackedObject{environment},
    _topicOrd{topicOrd},
    _receivers{receivers->weak_from_this()},
    _callback{std::move(callback)} {

}

pubsub::TopicReceivers::TopicReceivers(data::Environment &environment, data::Handle topicOrd, pubsub::LocalTopics *topics) :
    _environment{environment}, _topicOrd{topicOrd}, _topics {topics->weak_from_this()} {
}

void pubsub::TopicReceivers::cleanup() {
    // scoped lock
    {
        std::unique_lock guard{_environment.sharedLocalTopicsMutex};
        (void)std::remove_if(_receivers.begin(), _receivers.end(), [&](const auto &item) {
            return item.expired();
        });
    }
    // lock must be released before this step
    if (_receivers.empty() && !_topics.expired()) {
        std::shared_ptr<pubsub::LocalTopics> topics{_topics};
        topics->cleanup();
    }
}

void pubsub::LocalTopics::cleanup() {
    std::unique_lock guard{_environment.sharedLocalTopicsMutex};
    for (auto i = _topics.begin(); i != _topics.end(); ++i) {
        if (i->second->isEmpty()) {
            _topics.erase(i);
        }
    }
}

std::shared_ptr<pubsub::TopicReceiver> pubsub::TopicReceivers::newReceiver(std::unique_ptr<pubsub::AbstractCallback> &callback) {
    std::shared_ptr<pubsub::TopicReceiver> receiver {std::make_shared<pubsub::TopicReceiver>(_environment, _topicOrd, this, callback)};
    std::unique_lock guard{_environment.sharedLocalTopicsMutex};
    _receivers.push_back(receiver);
    return receiver;
}

std::shared_ptr<pubsub::TopicReceivers> pubsub::LocalTopics::testAndGetReceivers(data::Handle topicOrd) {
    std::shared_lock guard{_environment.sharedLocalTopicsMutex};
    auto i = _topics.find(topicOrd);
    if (i == _topics.end()) {
        return {};
    } else {
        return i->second;
    }
}

std::shared_ptr<pubsub::TopicReceivers> pubsub::LocalTopics::getOrCreateReceivers(data::Handle topicOrd) {
    std::shared_ptr<pubsub::TopicReceivers> receivers = testAndGetReceivers(topicOrd);
    if (receivers) {
        return receivers;
    }
    std::unique_lock guard{_environment.sharedLocalTopicsMutex};
    auto i = _topics.find(topicOrd);
    if (i != _topics.end()) {
        // rare edge case
        return i->second;
    }
    receivers = std::make_shared<pubsub::TopicReceivers>(_environment, topicOrd, this);
    _topics[topicOrd] = receivers;
    return receivers;
}

std::shared_ptr<data::ObjectAnchor> pubsub::LocalTopics::subscribe(data::Handle anchor, data::Handle topicOrd, std::unique_ptr<pubsub::AbstractCallback> & callback) {
    auto root = _environment.handleTable.getObject<data::TrackingScope>(anchor);
    std::shared_ptr<pubsub::TopicReceivers> receivers = getOrCreateReceivers(topicOrd);
    std::shared_ptr<pubsub::TopicReceiver> receiver = receivers->newReceiver(callback);
    return root->anchor(receiver.get()); // if handle or root goes away, unsubscribe
}

void pubsub::LocalTopics::insertCallQueue(std::shared_ptr<tasks::Task> & task, data::Handle topicOrd) {
    std::shared_ptr<pubsub::TopicReceivers> receivers = testAndGetReceivers(topicOrd);
    if (receivers == nullptr || receivers->isEmpty()) {
        return;
    }
    std::vector<std::shared_ptr<pubsub::TopicReceiver>> callOrder;
    receivers->getCallOrder(callOrder);
    for (const auto& i : callOrder) {
        task->addSubtask(std::move(i->toSubTask(task)));
    }
}

void pubsub::TopicReceivers::getCallOrder(std::vector<std::shared_ptr<pubsub::TopicReceiver>> & callOrder) {
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

class ReceiverSubTask : public tasks::SubTask {
private:
    std::shared_ptr<pubsub::TopicReceiver> _receiver;
public:
    explicit ReceiverSubTask(std::shared_ptr<pubsub::TopicReceiver> & receiver) :
        _receiver{receiver} {
    }
    std::shared_ptr<data::Structish> runInThread(const std::shared_ptr<tasks::Task> &task, const std::shared_ptr<data::Structish> &dataIn) override;
};

std::unique_ptr<tasks::SubTask> pubsub::TopicReceiver::toSubTask(std::shared_ptr<tasks::Task> & task) {
    std::shared_lock guard{_environment.sharedLocalTopicsMutex};
    std::shared_ptr<pubsub::TopicReceiver> receiver {std::static_pointer_cast<pubsub::TopicReceiver>(shared_from_this())};
    std::unique_ptr<tasks::SubTask> subTask {new ReceiverSubTask(receiver) };
    return subTask;
}

std::shared_ptr<data::Structish> ReceiverSubTask::runInThread(const std::shared_ptr<tasks::Task> &task, const std::shared_ptr<data::Structish> &dataIn) {
    return _receiver->runInTaskThread(task, dataIn);
}

std::shared_ptr<data::Structish> pubsub::TopicReceiver::runInTaskThread(const std::shared_ptr<tasks::Task> &task, const std::shared_ptr<data::Structish> &dataIn) {
    data::Handle dataHandle {task->anchor(dataIn.get()) };
    data::Handle resp = _callback->operator()(task->getSelf(), _topicOrd, dataHandle);
    std::shared_ptr<data::Structish> respData;
    if (resp) {
        respData = _environment.handleTable.getObject<data::Structish>(resp);
    }
    return respData;
}

class CompletionSubTask : public tasks::SubTask
{
private:
    data::Handle _topicOrd;
    std::unique_ptr<pubsub::AbstractCallback> _callback;
public:
    explicit CompletionSubTask(data::Handle topicOrd, std::unique_ptr<pubsub::AbstractCallback> &callback) :
            _topicOrd{topicOrd},
            _callback{std::move(callback)} {
    }
    std::shared_ptr<data::Structish> runInThread(const std::shared_ptr<tasks::Task> &task, const std::shared_ptr<data::Structish> &result) override;
};

std::shared_ptr<data::Structish> CompletionSubTask::runInThread(const std::shared_ptr<tasks::Task> &task, const std::shared_ptr<data::Structish> &result) {
    data::Handle dataHandle {task->anchor(result.get()) };
    (void)_callback->operator()(task->getSelf(), _topicOrd, dataHandle);
    return nullptr;
}

void pubsub::LocalTopics::applyCompletion(std::shared_ptr<tasks::Task> &task, data::Handle topicOrd,
                                          std::unique_ptr<pubsub::AbstractCallback> &callback) {
    if (!callback) {
        return;
    }
    std::unique_ptr<tasks::SubTask> subTask {new CompletionSubTask(topicOrd, callback) };
    task->setCompletion(subTask);
}
