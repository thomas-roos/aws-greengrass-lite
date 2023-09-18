#include "task.h"

thread_local Handle Task::_threadTask {Handle::nullHandle};

std::shared_ptr<Anchored> TaskManager::createTask() {
    auto task{ std::make_shared<Task>(_environment)};
    auto taskAnchor {anchor(task.get())};
    task->setSelf(taskAnchor->getHandle());
    return taskAnchor;
}

void TaskManager::queueAsyncTask(std::shared_ptr<Task> &task) {
    std::unique_lock guard{_workerPoolMutex};
    _backlog.push_back(task);
}

bool TaskManager::allocateNextWorker() {
    std::unique_lock guard{_workerPoolMutex};
    if (_backlog.empty()) {
        return true; // backlog is empty
    }
    if (_idleWorkers.empty()) {
        if (_busyWorkers.size() >= _maxWorkers) {
            return false; // run out of workers
        }
        // allocate a new worker - it will be tasked with picking up next task
        // TODO: add some kind of knowledge of workers starting
        // code as is can cause a scramble
        std::shared_ptr<TaskManager> pool{std::static_pointer_cast<TaskManager>(this->shared_from_this())};
        std::shared_ptr<TaskWorker> worker{std::make_shared<TaskWorker>(_environment, pool)};
        _busyWorkers.push_back(worker);
        worker->waken();
        return true;
    } else {
        std::shared_ptr<TaskWorker> worker{_idleWorkers.back()};
        _idleWorkers.pop_back();
        _busyWorkers.push_back(worker);
        worker->waken();
        return true;
    }
}

std::shared_ptr<Task> TaskManager::acquireTask(TaskWorker * worker) {
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

TaskWorker::TaskWorker(Environment & environment, std::shared_ptr<TaskManager> & pool) :
    _environment{environment}, _pool{pool} {
    _thread = std::thread(&TaskWorker::runner, this);
}

void TaskWorker::runner() {
    std::shared_ptr<Task> task = pickupTask();
    while(task) {
        task->runInThread();
        task = pickupTask();
    }
}

std::shared_ptr<Task> TaskWorker::pickupTask() {
    for (;;) {
        if (_pool.expired()) {
            return nullptr;
        }
        // TODO: possible race / handle with exception handling
        std::shared_ptr<TaskManager> pool = _pool.lock();
        std::shared_ptr<Task> task = pool->acquireTask(this);
        if (task != nullptr) {
            return task;
        }
        std::unique_lock guard(_mutex);
        if (_shutdown) {
            return nullptr;
        }
        _wake.wait(guard);
    }
}

void Task::markTaskComplete(std::shared_ptr<SharedStruct> &result) {
    std::unique_lock guard {_mutex};
    _subtasks.clear();
    _data = result;
    _completed = true;
}

std::unique_ptr<SubTask> Task::removeSubtask() {
    std::unique_lock guard {_mutex};
    if (_subtasks.empty()) {
        return nullptr;
    }
    std::unique_ptr<SubTask> subTask { std::move(_subtasks.front()) };
    _subtasks.pop_front();
    return subTask;
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

void Task::runInThread() {
    // TODO: Need to handle thread affinity and thread re-use
    std::shared_ptr<Task> taskObj { std::static_pointer_cast<Task>(shared_from_this()) };
    ThreadSelf threadSelf(getSelf());
    std::shared_ptr<SharedStruct> dataIn { getData() };
    std::shared_ptr<SharedStruct> dataOut { runInThreadCallNext(taskObj, dataIn) };
    if (_finalize) {
        // end of call chain
        _finalize->runInThread(taskObj, dataOut);
    }
    markTaskComplete(dataOut);
    _waiters.notify_all();
}

bool Task::waitForCompletion(time_t terminateTime) {
    // TODO: Opportunity for task stealing to this thread
    // TODO: Timeout
    std::unique_lock guard {_mutex};
    while(!_completed) {
        _waiters.wait(guard);
    }
    return true;
}

std::shared_ptr<SharedStruct> Task::runInThreadCallNext(std::shared_ptr<Task> & task, std::shared_ptr<SharedStruct> & dataIn) {
    // TODO: Need to handle thread affinity and thread re-use
    for (;;) {
        std::unique_ptr<SubTask> subTask { removeSubtask() };
        if (!subTask) {
            return nullptr;
        }
        std::shared_ptr<SharedStruct> dataOut { subTask->runInThread(task, dataIn) };
        if (dataOut != nullptr) {
            return dataOut;
        }
    }
}
