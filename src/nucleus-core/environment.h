#pragma once
#include "handle_table.h"
#include "string_table.h"
#include <shared_mutex>

struct Environment {
    HandleTable handleTable;
    StringTable stringTable;
    std::shared_mutex sharedStructMutex;
    std::mutex sharedRootsMutex;
    std::shared_mutex sharedLocalTopicsMutex;

    virtual time_t relativeToAbsoluteTime(time_t relTime);
};
