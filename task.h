#pragma once
#include "environment.h"
#include "handle_table.h"
#include "local_topics.h"
#include <vector>
#include <mutex>
#include <list>
#include <thread>
#include <condition_variable>

//
// Context to track work to do
//
class TaskManager;
class Task;

class SubTask : public std::enable_shared_from_this<SubTask> {
public:
    virtual ~SubTask() = default;
    virtual std::shared_ptr<SharedStruct> runInThread(std::shared_ptr<Task> & task, std::shared_ptr<SharedStruct> & dataIn) = 0;
};

class Task : public AnchoredWithRoots {
    friend class TaskManager;
private:
    mutable std::mutex _mutex;
    std::shared_ptr<SharedStruct> _data;
    std::unique_ptr<SubTask> _finalize;
    std::list<std::unique_ptr<SubTask>> _subtasks;
    std::condition_variable _waiters;
    Handle _self;
    time_t _timeout;
    bool _completed;
    static thread_local Handle _threadTask;

public:
    explicit Task(Environment & environment) :
        AnchoredWithRoots {environment},
        _timeout{-1} {
    }
    void setSelf(Handle self) {
        std::unique_lock guard{_mutex};
        _self = self;
    }
    Handle getSelf() {
        std::unique_lock guard{_mutex};
        return _self;
    }
    std::shared_ptr<SharedStruct> getData() {
        std::unique_lock guard{_mutex};
        return _data;
    }
    void setData(std::shared_ptr<SharedStruct> & newData) {
        std::unique_lock guard{_mutex};
        _data = newData;
    }
    void markTaskComplete(std::shared_ptr<SharedStruct> & result);
    static Handle getThreadSelf() {
        return _threadTask;
    }
    static Handle getSetThreadSelf(Handle h) {
        Handle old = _threadTask;
        _threadTask = h;
        return old;
    }

    std::unique_ptr<SubTask> removeSubtask();
    void addSubtask(std::unique_ptr<SubTask> & subTask);
    void setCompletion(std::unique_ptr<SubTask> & finalize) {
        std::unique_lock guard{_mutex};
        _finalize = std::move(finalize);
    }
    void setTimeout(time_t terminateTime) {
        std::unique_lock guard{_mutex};
        _timeout = terminateTime;
    }
    void runInThread();
    bool waitForCompletion(time_t terminateTime);
    std::shared_ptr<SharedStruct> runInThreadCallNext(std::shared_ptr<Task> & task, std::shared_ptr<SharedStruct> & dataIn);
};

class TaskManager;
class TaskWorker : public std::enable_shared_from_this<TaskWorker> {
private:
    Environment & _environment;
    std::weak_ptr<TaskManager> _pool;
    std::thread _thread;
    std::mutex _mutex;
    std::condition_variable _wake;
    bool _shutdown {false};
public:
    explicit TaskWorker(Environment & environment, std::shared_ptr<TaskManager> & pool);
    void runner();

    void shutdown() {
        std::unique_lock guard(_mutex);
        _shutdown = true;
        _wake.notify_one();
    }

    void waken() {
        std::unique_lock guard(_mutex);
        _wake.notify_one();
    }

    std::shared_ptr<Task> pickupTask();

    bool isShutdown() {
        std::unique_lock guard(_mutex);
        return _shutdown;
    }
};

class TaskManager : public AnchoredWithRoots {
private:
    std::mutex _workerPoolMutex;
    std::list<std::shared_ptr<TaskWorker>> _busyWorkers;
    std::list<std::shared_ptr<TaskWorker>> _idleWorkers;
    std::list<std::shared_ptr<Task>> _backlog;
    const int _maxWorkers {5}; // TODO, from configuration

public:
    explicit TaskManager(Environment & environment) : AnchoredWithRoots{environment} {
    }

    std::shared_ptr<Anchored> createTask();
    std::shared_ptr<Task> acquireTask(TaskWorker * worker);
    bool allocateNextWorker();
    void queueAsyncTask(std::shared_ptr<Task> & task);
};
