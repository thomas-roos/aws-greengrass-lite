#include "../data/globals.h"
#include <c_api.h>

uint32_t ggapiClaimThread() {
    Global & global = Global::self();
    std::shared_ptr<FixedTaskThread> thread {std::make_shared<FixedTaskThread>(global.environment, global.taskManager)};
    return Handle{thread->claimFixedThread()}.asInt();
}

void ggapiReleaseThread() {
    std::shared_ptr<TaskThread> thread = FixedTaskThread::getThreadContext();
    thread->releaseFixedThread();
}

uint32_t ggapiGetCurrentTask(void) {
    return Task::getThreadSelf().asInt();
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
