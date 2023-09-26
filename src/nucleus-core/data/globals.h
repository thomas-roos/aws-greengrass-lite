#pragma once
#include "environment.h"
#include "../tasks/task.h"
#include "../pubsub/local_topics.h"
#include "../plugins/plugin_loader.h"

struct Global {
    Environment environment;
    std::shared_ptr<TaskManager> taskManager {std::make_shared<TaskManager>(environment)};
    std::shared_ptr<LocalTopics> lpcTopics {std::make_shared<LocalTopics>(environment)};
    std::shared_ptr<PluginLoader> loader {std::make_shared<PluginLoader>(environment)};

    static Global & self() {
        static Global global;
        return global;
    }

    static Environment & env() {
        return self().environment;
    }
};
