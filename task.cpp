#include "task.h"

thread_local Handle Task::_threadTask {Handle::nullHandle};
thread_local std::weak_ptr<TaskThread> TaskThread::_threadContext;

std::shared_ptr<Anchored> TaskManager::createTask() {
    auto task{ std::make_shared<Task>(_environment)};
    auto taskAnchor {anchor(task.get())};
    task->setSelf(taskAnchor->getHandle());
    return taskAnchor;
}

void TaskManager::queueTask(const std::shared_ptr<Task> &task) {
    std::shared_ptr<TaskThread> affinity = task->getThreadAffinity();
    if (affinity) {
        // thread affinity - need to assign task to the specified thread
        // e.g. event thread of a given WASM VM
        affinity->queueTask(task);
        affinity->waken();
    } else {
        // no affinity, just add to backlog, a worker will pick this up
        std::unique_lock guard{_workerPoolMutex};
        _backlog.push_back(task);
    }
}

void TaskThread::queueTask(const std::shared_ptr<Task> &task) {
    std::unique_lock guard{_mutex};
    _tasks.push_back(task);
}

bool TaskManager::allocateNextWorker() {
    std::unique_lock guard{_workerPoolMutex};
    if (_backlog.empty()) {
        return true; // backlog is empty (does not include thread-assigned tasks)
    }
    if (_idleWorkers.empty()) {
        if (_busyWorkers.size() >= _maxWorkers) {
            return false; // run out of workers
        }
        // allocate a new worker - it will be tasked with picking up next task
        // TODO: add some kind of knowledge of workers starting
        // code as is can cause a scramble
        std::shared_ptr<TaskManager> pool{std::static_pointer_cast<TaskManager>(this->shared_from_this())};
        std::shared_ptr<TaskPoolWorker> worker{std::make_shared<TaskPoolWorker>(_environment, pool)};
        _busyWorkers.push_back(worker);
        worker->waken();
        return true;
    } else {
        std::shared_ptr<TaskPoolWorker> worker{_idleWorkers.back()};
        _idleWorkers.pop_back();
        _busyWorkers.push_back(worker);
        worker->waken();
        return true;
    }
}
std::shared_ptr<Task> TaskManager::acquireTaskWhenStealing(TaskThread *worker, const std::shared_ptr<Task> & priorityTask) {
    std::unique_lock guard{_workerPoolMutex};
    if (_backlog.empty()) {
        return nullptr;
    }
    for (auto i = _backlog.begin(); i != _backlog.end(); ++i) {
        if (*i == priorityTask) {
            _backlog.erase(i);
            return priorityTask; // claim priority task
        }
    }
    // permitted to take any backlog task if blocked
    std::shared_ptr<Task> work {_backlog.front()};
    _backlog.pop_front();
    return work;
}

std::shared_ptr<Task> TaskManager::acquireTaskForWorker(TaskThread *worker) {
    std::unique_lock guard{_workerPoolMutex};
    if (_backlog.empty()) {
        // backlog is empty, need to idle this worker
        for (auto i = _busyWorkers.begin(); i != _busyWorkers.end(); ++i) {
            if (i->get() == worker) {
                _idleWorkers.push_back(*i);
                _busyWorkers.erase(i);
                break;
            }
        }
        return nullptr; // backlog is empty
    }
    std::shared_ptr<Task> work {_backlog.front()};
    _backlog.pop_front();
    return work;
}

void Task::addSubtask(std::unique_ptr<SubTask> &subTask) {
    std::unique_lock guard {_mutex};
    _subtasks.push_back(std::move(subTask));
}

void TaskThread::bindThreadContext() {
    _threadContext = weak_from_this();
}

std::shared_ptr<TaskThread> TaskThread::getThreadContext() {
    // TaskThread for current thread
    return _threadContext.lock();
}

void FixedTaskThread::setDefaultTask(const std::shared_ptr<Anchored> &task) {
    std::unique_lock guard{_mutex};
    _defaultTask = task;
}

void FixedTaskThread::protect() {
    std::unique_lock guard{_mutex};
    _protectThread = shared_from_this();
}

void FixedTaskThread::unprotect() {
    std::unique_lock guard{_mutex};
    _protectThread = nullptr;
}

void FixedTaskThread::bindThreadContext(const std::shared_ptr<Anchored> & task) {
    // Run on target thread
    setDefaultTask(task);
    task->getObject<Task>()->getSetThreadSelf(Handle{task});
    TaskThread::bindThreadContext();
}

TaskThread::TaskThread(Environment & environment, const std::shared_ptr<TaskManager> &pool) :
        _environment{environment}, _pool{pool} {
}

TaskPoolWorker::TaskPoolWorker(Environment & environment, const std::shared_ptr<TaskManager> &pool) :
        TaskThread(environment, pool) {
    _thread = std::thread(&TaskPoolWorker::runner, this);
}

void TaskPoolWorker::runner() {
    bindThreadContext();
    while (!isShutdown()) {
        std::shared_ptr<Task> task = pickupTask();
        if (task) {
            task->runInThread();
        } else {
            stall();
        }
    }
}

void TaskThread::taskStealing(const std::shared_ptr<Task> & blockingTask) {
    // this loop is entered when we have an associated task
    while (!blockingTask->isCompleted()) {
        std::shared_ptr<Task> task = pickupTask(blockingTask);
        if (task) {
            task->runInThread();
        } else {
            // stall until task unblocked
            // TODO: right now this will ignore all free-task workers
            stall();
        }
    }
}

std::shared_ptr<Task> TaskThread::pickupAffinitizedTask() {
    std::unique_lock guard {_mutex};
    if (_tasks.empty()) {
        return nullptr;
    }
    std::shared_ptr<Task> next = _tasks.front();
    _tasks.pop_front();
    return next;
}

std::shared_ptr<Task> TaskThread::pickupPoolTask(const std::shared_ptr<Task> & blockingTask) {
    if (_pool.expired()) {
        return nullptr;
    }
    // TODO: possible race / handle with exception handling
    std::shared_ptr<TaskManager> pool = _pool.lock();
    return pool->acquireTaskWhenStealing(this, blockingTask);
}

std::shared_ptr<Task> TaskThread::pickupPoolTask() {
    if (_pool.expired()) {
        return nullptr;
    }
    // TODO: possible race / handle with exception handling
    std::shared_ptr<TaskManager> pool = _pool.lock();
    return pool->acquireTaskForWorker(this);
}

std::shared_ptr<Task> TaskThread::pickupTask() {
    std::shared_ptr<Task> task { pickupAffinitizedTask() };
    if (task) {
        return task;
    }
    return pickupPoolTask();
}

std::shared_ptr<Task> TaskThread::pickupTask(const std::shared_ptr<Task> & blockingTask) {
    std::shared_ptr<Task> task { pickupAffinitizedTask() };
    if (task) {
        return task;
    }
    return pickupPoolTask(blockingTask);
}

void Task::markTaskComplete() {
    std::unique_lock guard {_mutex};
    _lastStatus = Completed;
    // all blocked threads are in process of task stealing, make sure they are not blocked idle
    for (const auto& i : _blockedThreads) {
        i->waken();
    }
}

bool Task::isCompleted() {
    std::unique_lock guard {_mutex};
    return _lastStatus == Completed;
}

Task::Status Task::removeSubtask(std::unique_ptr<SubTask> & subTask) {
    std::unique_lock guard {_mutex};
    if (_subtasks.empty()) {
        return NoSubTasks;
    }
    std::shared_ptr<TaskThread> affinity = _subtasks.front()->getAffinity();
    if (affinity && affinity != TaskThread::getThreadContext()) {
        return SwitchThread; // task cannot run on current thread
    }
    subTask = std::move(_subtasks.front());
    _subtasks.pop_front();
    return Running;
}

class ThreadSelf {
private:
    Handle _oldHandle;
public:
    explicit ThreadSelf(Handle newHandle) {
        _oldHandle = Task::getSetThreadSelf(newHandle);
    }
    ~ThreadSelf() {
        Task::getSetThreadSelf(_oldHandle);
    }
};

std::shared_ptr<TaskThread> Task::getThreadAffinity() {
    // What thread is allowed to handle the next sub-task?
    std::unique_lock guard {_mutex};
    if (_subtasks.empty()) {
        return nullptr;
    }
    return _subtasks.front()->getAffinity();
}

Task::Status Task::runInThread() {
    std::shared_ptr<Task> taskObj { std::static_pointer_cast<Task>(shared_from_this()) };
    ThreadSelf threadSelf(getSelf());
    std::shared_ptr<SharedStruct> dataIn { getData() };
    std::shared_ptr<SharedStruct> dataOut;
    Status status = runInThreadCallNext(taskObj, dataIn, dataOut);
    while (status == NoSubTasks) {
        status = finalizeTask(dataOut);
        if (status == Completed) {
            markTaskComplete();
            return status; // need to switch thread to continue
        } else if (status == Finalizing) {
            status = runInThreadCallNext(taskObj, dataOut, dataOut);
        } else {
            break;
        }
    }
    return status;
}

Task::Status Task::finalizeTask(const std::shared_ptr<SharedStruct> & data) {
    std::unique_lock guard{_mutex};
    if (_lastStatus == Finalizing) {
        if (_subtasks.empty()) {
            return Completed; // completed finalization step
        } else {
            return SwitchThread; // assume we may have to switch thread to finalize
        }
    }
    _subtasks.clear(); // if data provided, abort early
    _data = data; // finalization data
    if (_finalize) {
        // move finalization function to end of call chain
        _subtasks.push_back(std::move(_finalize));
    }
    return _lastStatus = Finalizing; // perform finalization step in same thread if possible
}

void Task::addBlockedThread(const std::shared_ptr<TaskThread> & blockedThread) {
    std::unique_lock guard{_mutex};
    _blockedThreads.push_back(blockedThread); // threads blocked and in process of task stealing
}

bool Task::waitForCompletion(time_t terminateTime) {
    // TODO: Timeout
    std::shared_ptr<TaskThread> thisThread = TaskThread::getThreadContext();
    addBlockedThread(thisThread);
    thisThread->taskStealing(shared_from_this());
    return isCompleted();
}

Task::Status Task::runInThreadCallNext(const std::shared_ptr<Task> & task, const std::shared_ptr<SharedStruct> & dataIn, std::shared_ptr<SharedStruct> & dataOut) {
    for (;;) {
        std::unique_ptr<SubTask> subTask;
        Status status = removeSubtask(subTask);
        if (status != Running) {
            return status;
        }
        dataOut = subTask->runInThread(task, dataIn);
        if (dataOut != nullptr) {
            return NoSubTasks;
        }
    }
}
