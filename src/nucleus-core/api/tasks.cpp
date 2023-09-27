#include "../data/globals.h"
#include <c_api.h>

uint32_t ggapiClaimThread() {
    data::Global & global = data::Global::self();
    std::shared_ptr<tasks::FixedTaskThread> thread {std::make_shared<tasks::FixedTaskThread>(global.environment, global.taskManager)};
    return thread->claimFixedThread().getHandle().asInt();
}

void ggapiReleaseThread() {
    std::shared_ptr<tasks::TaskThread> thread = tasks::FixedTaskThread::getThreadContext();
    thread->releaseFixedThread();
}

uint32_t ggapiGetCurrentTask(void) {
    return tasks::Task::getThreadSelf().asInt();
}

uint32_t ggapiWaitForTaskCompleted(uint32_t asyncTask, int32_t timeout) {
    data::Global & global = data::Global::self();
    data::Handle parentTask { tasks::Task::getThreadSelf() };
    std::shared_ptr<tasks::Task> parentTaskObj { global.environment.handleTable.getObject<tasks::Task>(parentTask) };
    std::shared_ptr<tasks::Task> asyncTaskObj { global.environment.handleTable.getObject<tasks::Task>(data::ObjHandle{asyncTask}) };
    ExpireTime expireTime = global.environment.translateExpires(timeout);
    if (asyncTaskObj->waitForCompletion(expireTime)) {
        return parentTaskObj->anchor(asyncTaskObj->getData()).getHandle().asInt();
    } else {
        return 0;
    }
}
