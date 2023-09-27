#pragma once
#include "data/safe_handle.h"
#include "data/handle_table.h"
#include "data/environment.h"
#include "data/shared_struct.h"
#include <map>
#include <string>
#include <vector>
#include <functional>

namespace tasks {
    class Task;
    class SubTask;
}

namespace data {
    class Structish;
}

namespace pubsub {
    class TopicReceivers;
    class LocalTopics;

    class AbstractCallback {
    public:
        virtual ~AbstractCallback() = default;
        virtual data::Handle operator()(data::Handle taskHandle, data::Handle topicOrd, data::Handle dataStruct) = 0;
    };

    class TopicReceiver : public data::TrackedObject {
    private:
        data::Handle _topicOrd;
        std::weak_ptr<TopicReceivers> _receivers;
        std::unique_ptr<AbstractCallback> _callback;

    public:
        explicit TopicReceiver(data::Environment & environment, data::Handle topicOrd, TopicReceivers * receivers, std::unique_ptr<AbstractCallback> & callback);
        ~TopicReceiver() override;
        std::unique_ptr<tasks::SubTask> toSubTask(std::shared_ptr<tasks::Task> & task);
        std::shared_ptr<data::Structish> runInTaskThread(const std::shared_ptr<tasks::Task> &task, const std::shared_ptr<data::Structish> &dataIn);
    };

    class TopicReceivers : public std::enable_shared_from_this<TopicReceivers> {
    private:
        data::Environment & _environment;
        data::Handle _topicOrd;
        std::weak_ptr<LocalTopics> _topics;
        std::vector<std::weak_ptr<TopicReceiver>> _receivers;
    public:
        TopicReceivers(data::Environment &environment, data::Handle topicOrd, LocalTopics *topics);
        void cleanup();
        bool isEmpty() {
            return _receivers.empty();
        }
        std::shared_ptr<TopicReceiver> newReceiver(std::unique_ptr<AbstractCallback> & callback);
        void getCallOrder(std::vector<std::shared_ptr<TopicReceiver>> & callOrder);
    };

    class LocalTopics : public std::enable_shared_from_this<LocalTopics> {
    private:
        data::Environment & _environment;
        std::map<data::Handle, std::shared_ptr<TopicReceivers>, data::Handle::CompLess> _topics;
    public:
        explicit LocalTopics(data::Environment & environment) : _environment{environment} {
        }
        void cleanup();
        std::shared_ptr<TopicReceivers> testAndGetReceivers(data::Handle topicOrd);
        std::shared_ptr<TopicReceivers> getOrCreateReceivers(data::Handle topicOrd);
        std::shared_ptr<data::ObjectAnchor> subscribe(data::Handle anchor, data::Handle topicOrd, std::unique_ptr<AbstractCallback> & callback);
        static void applyCompletion(std::shared_ptr<tasks::Task> & task, data::Handle topicOrd, std::unique_ptr<AbstractCallback> & callback);
        void insertCallQueue(std::shared_ptr<tasks::Task> & task, data::Handle topicOrd);
    };
}

