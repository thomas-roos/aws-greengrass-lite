#pragma once
#include "handle_table.h"
#include "string_table.h"
#include "config.h"
#include "expire_time.h"
#include <shared_mutex>

struct Environment {
    HandleTable handleTable;
    StringTable stringTable;
    config::Manager configManager;
    std::shared_mutex sharedLocalTopicsMutex;
    std::mutex cycleCheckMutex;

    virtual ExpireTime translateExpires(int32_t delta);
};
