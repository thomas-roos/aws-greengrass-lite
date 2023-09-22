#pragma once
#include "environment.h"
#include "handle_table.h"
#include "local_topics.h"
#include <vector>
#include <mutex>
#include <list>
#include <set>
#include <thread>
#include <condition_variable>

//
// Context to track work to do
//
class TaskManager;
class TaskThread;
class TaskPoolWorker;
class Task;

class SubTask : public std::enable_shared_from_this<SubTask> {
protected:
    std::shared_ptr<TaskThread> _threadAffinity;
public:
    virtual ~SubTask() = default;
    virtual std::shared_ptr<SharedStruct> runInThread(const std::shared_ptr<Task> &task, const std::shared_ptr<SharedStruct> &dataIn) = 0;
    void setAffinity(const std::shared_ptr<TaskThread> & affinity);
    std::shared_ptr<TaskThread> getAffinity();
};

class Task : public AnchoredWithRoots {
public:
    enum Status {
        Running,
        NoSubTasks,
        Finalizing,
        SwitchThread,
        Completed
    };
    friend class TaskManager;
private:
    std::shared_ptr<SharedStruct> _data;
    std::unique_ptr<SubTask> _finalize;
    std::list<std::unique_ptr<SubTask>> _subtasks;
    std::list<std::shared_ptr<TaskThread>> _blockedThreads;
    Handle _self;
    time_t _timeout;
    Status _lastStatus {Running};
    static thread_local Handle _threadTask;

public:

    explicit Task(Environment & environment) :
        AnchoredWithRoots {environment},
        _timeout{-1} {
    }
    std::shared_ptr<Task> shared_from_this() {
        return std::static_pointer_cast<Task>(AnchoredWithRoots::shared_from_this());
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
    void setData(const std::shared_ptr<SharedStruct> &newData) {
        std::unique_lock guard{_mutex};
        _data = newData;
    }
    std::shared_ptr<TaskThread> getThreadAffinity();
    void markTaskComplete();
    static Handle getThreadSelf() {
        return _threadTask;
    }
    static Handle getSetThreadSelf(Handle h) {
        Handle old = _threadTask;
        _threadTask = h;
        return old;
    }

    Status removeSubtask(std::unique_ptr<SubTask> & subTask);
    void addSubtask(std::unique_ptr<SubTask> subTask);
    void setCompletion(std::unique_ptr<SubTask> & finalize) {
        std::unique_lock guard{_mutex};
        _finalize = std::move(finalize);
    }
    void setTimeout(time_t terminateTime) {
        std::unique_lock guard{_mutex};
        _timeout = terminateTime;
    }
    Status runInThread();
    bool waitForCompletion(time_t terminateTime);
    Status runInThreadCallNext(const std::shared_ptr<Task> & task, const std::shared_ptr<SharedStruct> & dataIn, std::shared_ptr<SharedStruct> & dataOut);

    void addBlockedThread(const std::shared_ptr<TaskThread> &blockedThread);

    bool isCompleted();

    Status finalizeTask(const std::shared_ptr<SharedStruct> &data);
};

class TaskThread : public std::enable_shared_from_this<TaskThread> {
    // mix-in representing either a worker thread or fixed thread
protected:
    Environment & _environment;
    std::weak_ptr<TaskManager> _pool;
    std::list<std::shared_ptr<Task>> _tasks;
    std::mutex _mutex;
    std::condition_variable _wake;
    bool _shutdown {false};
    static thread_local std::weak_ptr<TaskThread> _threadContext;
    void bindThreadContext();
public:
    explicit TaskThread(Environment & environment, const std::shared_ptr<TaskManager> &pool);
    void queueTask(const std::shared_ptr<Task> & task);
    std::shared_ptr<Task> pickupAffinitizedTask();
    std::shared_ptr<Task> pickupPoolTask();
    std::shared_ptr<Task> pickupTask();

    static std::shared_ptr<TaskThread> getThreadContext();

    void shutdown() {
        std::unique_lock guard(_mutex);
        _shutdown = true;
        _wake.notify_one();
    }

    void stall() {
        std::unique_lock guard(_mutex);
        if (_shutdown) {
            return;
        }
        _wake.wait(guard);
    }

    void waken() {
        std::unique_lock guard(_mutex);
        _wake.notify_one();
    }

    bool isShutdown() {
        std::unique_lock guard(_mutex);
        return _shutdown;
    }

    void taskStealing(const std::shared_ptr<Task> & blockingTask);

    std::shared_ptr<Task> pickupTask(const std::shared_ptr<Task> &blockingTask);

    std::shared_ptr<Task> pickupPoolTask(const std::shared_ptr<Task> &blockingTask);
};

class TaskManager;
class TaskPoolWorker : public TaskThread {
private:
    std::thread _thread;
public:
    explicit TaskPoolWorker(Environment & environment, const std::shared_ptr<TaskManager> &pool);
    void runner();
    std::shared_ptr<TaskPoolWorker> shared_from_this() {
        return std::static_pointer_cast<TaskPoolWorker>(TaskThread::shared_from_this());
    }
};

class FixedTaskThread : public TaskThread {
protected:
    std::shared_ptr<Anchored> _defaultTask;
    std::shared_ptr<FixedTaskThread> _protectThread;
public:
    explicit FixedTaskThread(Environment & environment, const std::shared_ptr<TaskManager> &pool) :
        TaskThread(environment, pool) {
    }
    // Call this on the native thread
    void bindThreadContext(const std::shared_ptr<Anchored> & task);
    void setDefaultTask(const std::shared_ptr<Anchored> & task);
    void protect();
    void unprotect();
    std::shared_ptr<FixedTaskThread> shared_from_this() {
        return std::static_pointer_cast<FixedTaskThread>(TaskThread::shared_from_this());
    }
};

inline void SubTask::setAffinity(const std::shared_ptr<TaskThread> & affinity) {
    _threadAffinity = affinity;
}
inline std::shared_ptr<TaskThread> SubTask::getAffinity() {
    return _threadAffinity;
}

class TaskManager : public AnchoredWithRoots {
private:
    std::list<std::shared_ptr<TaskPoolWorker>> _busyWorkers; // assumes small pool, else std::set
    std::list<std::shared_ptr<TaskPoolWorker>> _idleWorkers; // LIFO
    std::list<std::shared_ptr<Task>> _backlog; // tasks with no thread affinity (assumed async)
    const int _maxWorkers {5}; // TODO, from configuration

public:
    explicit TaskManager(Environment & environment) : AnchoredWithRoots{environment} {
    }

    std::shared_ptr<Anchored> createTask();
    std::shared_ptr<Task> acquireTaskForWorker(TaskThread *worker);
    std::shared_ptr<Task> acquireTaskWhenStealing(TaskThread *worker, const std::shared_ptr<Task> & priorityTask);
    bool allocateNextWorker();
    void queueTask(const std::shared_ptr<Task> &task);
};
