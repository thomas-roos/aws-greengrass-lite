#pragma once
#include "data/environment.h"
#include "data/handle_table.h"
#include "pubsub/local_topics.h"
#include "data/safe_handle.h"
#include "data/shared_struct.h"
#include "pubsub/local_topics.h"
#include "expire_time.h"
#include <vector>
#include <mutex>
#include <list>
#include <set>
#include <thread>
#include <condition_variable>

namespace tasks {
    class TaskManager;
    class TaskThread;
    class TaskPoolWorker;
    class Task;
    class TaskManager;

    class SubTask : public std::enable_shared_from_this<SubTask> {
    protected:
        std::shared_ptr<TaskThread> _threadAffinity;
    public:
        SubTask() = default;
        SubTask(const SubTask&) = delete;
        SubTask(SubTask&&) = delete;
        SubTask& operator=(const SubTask&) = delete;
        SubTask& operator=(SubTask&&) = delete;
        virtual ~SubTask() = default;
        virtual std::shared_ptr<data::Structish> runInThread(const std::shared_ptr<Task> &task, const std::shared_ptr<data::Structish> &dataIn) = 0;
        void setAffinity(const std::shared_ptr<TaskThread> & affinity);
        std::shared_ptr<TaskThread> getAffinity();
    };

    class Task : public data::AnchoredWithRoots {
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
        std::shared_ptr<data::Structish> _data;
        std::unique_ptr<SubTask> _finalize;
        std::__cxx11::list<std::unique_ptr<SubTask>> _subtasks;
        std::__cxx11::list<std::shared_ptr<TaskThread>> _blockedThreads;
        data::Handle _self;
        ExpireTime _timeout;
        Status _lastStatus {Running};
        static thread_local data::Handle _threadTask;

    public:

        explicit Task(data::Environment & environment) :
            AnchoredWithRoots {environment},
            _timeout{ ExpireTime::fromNow(-1)} {
        }
        std::shared_ptr<Task> shared_from_this() {
            return std::static_pointer_cast<Task>(AnchoredWithRoots::shared_from_this());
        }
        void setSelf(data::Handle self) {
            std::unique_lock guard{_mutex};
            _self = self;
        }
        data::Handle getSelf() {
            std::unique_lock guard{_mutex};
            return _self;
        }
        std::shared_ptr<data::Structish> getData() {
            std::unique_lock guard{_mutex};
            return _data;
        }
        void setData(const std::shared_ptr<data::Structish> &newData) {
            std::unique_lock guard{_mutex};
            _data = newData;
        }
        std::shared_ptr<TaskThread> getThreadAffinity();
        void markTaskComplete();
        static data::Handle getThreadSelf() {
            return _threadTask;
        }
        static data::Handle getSetThreadSelf(data::Handle h) {
            data::Handle old = _threadTask;
            _threadTask = h;
            return old;
        }

        Status removeSubtask(std::unique_ptr<SubTask> & subTask);
        void addSubtask(std::unique_ptr<SubTask> subTask);
        void setCompletion(std::unique_ptr<SubTask> & finalize) {
            std::unique_lock guard{_mutex};
            _finalize = std::move(finalize);
        }
        void setTimeout(const ExpireTime & terminateTime) {
            std::unique_lock guard{_mutex};
            _timeout = terminateTime;
        }
        ExpireTime getTimeout() const {
            std::unique_lock guard{_mutex};
            return _timeout;
        }
        Status runInThread();
        bool waitForCompletion(const ExpireTime & terminateTime);
        Status runInThreadCallNext(const std::shared_ptr<Task> & task, const std::shared_ptr<data::Structish> & dataIn, std::shared_ptr<data::Structish> & dataOut);

        void addBlockedThread(const std::shared_ptr<TaskThread> &blockedThread);
        void removeBlockedThread(const std::shared_ptr<TaskThread> &blockedThread);

        bool isCompleted();
        bool willNeverComplete();

        Status finalizeTask(const std::shared_ptr<data::Structish> &data);
    };

    class TaskThread : public std::enable_shared_from_this<TaskThread> {
        // mix-in representing either a worker thread or fixed thread
    protected:
        data::Environment & _environment;
        std::weak_ptr<TaskManager> _pool;
        std::list<std::shared_ptr<Task>> _tasks;
        std::mutex _mutex;
        std::condition_variable _wake;
        bool _shutdown {false};
    //    static thread_local std::weak_ptr<TaskThread> _threadContext;
        static thread_local TaskThread * _threadContext;
        void bindThreadContext();
    public:
        explicit TaskThread(data::Environment & environment, const std::shared_ptr<TaskManager> &pool);
        TaskThread(const TaskThread &) = delete;
        TaskThread(TaskThread &&) = delete;
        TaskThread & operator=(const TaskThread &) = delete;
        TaskThread & operator=(TaskThread &&) = delete;
        virtual ~TaskThread() = default;
        void queueTask(const std::shared_ptr<Task> & task);
        std::shared_ptr<Task> pickupAffinitizedTask();
        std::shared_ptr<Task> pickupPoolTask();
        std::shared_ptr<Task> pickupTask();
        virtual void releaseFixedThread();

        static std::shared_ptr<TaskThread> getThreadContext();

        void shutdown() {
            std::unique_lock guard(_mutex);
            _shutdown = true;
            _wake.notify_one();
        }

        void stall(const ExpireTime & end) {
            std::unique_lock guard(_mutex);
            if (_shutdown) {
                return;
            }
            _wake.wait_for(guard, end.remaining());
        }

        void waken() {
            std::unique_lock guard(_mutex);
            _wake.notify_one();
        }

        bool isShutdown() {
            std::unique_lock guard(_mutex);
            return _shutdown;
        }

        void taskStealing(const std::shared_ptr<Task> & blockingTask, const ExpireTime & end);

        std::shared_ptr<Task> pickupTask(const std::shared_ptr<Task> &blockingTask);

        std::shared_ptr<Task> pickupPoolTask(const std::shared_ptr<Task> &blockingTask);
    };

    class TaskPoolWorker : public TaskThread {
    private:
        std::thread _thread;
    public:
        explicit TaskPoolWorker(data::Environment & environment, const std::shared_ptr<TaskManager> &pool);
        void runner();
        std::shared_ptr<TaskPoolWorker> shared_from_this() {
            return std::static_pointer_cast<TaskPoolWorker>(TaskThread::shared_from_this());
        }
    };

    class FixedTaskThread : public TaskThread {
    protected:
        std::shared_ptr<data::Anchored> _defaultTask;
        std::shared_ptr<FixedTaskThread> _protectThread;
    public:
        explicit FixedTaskThread(data::Environment & environment, const std::shared_ptr<TaskManager> &pool) :
            TaskThread(environment, pool) {
        }
        // Call this on the native thread
        void bindThreadContext(const std::shared_ptr<data::Anchored> & task);
        void setDefaultTask(const std::shared_ptr<data::Anchored> & task);
        std::shared_ptr<data::Anchored> getDefaultTask();
        void protect();
        void unprotect();
        std::shared_ptr<data::Anchored> claimFixedThread();
        void releaseFixedThread() override;
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

    class TaskManager : public data::AnchoredWithRoots {
    private:
        std::list<std::shared_ptr<TaskPoolWorker>> _busyWorkers; // assumes small pool, else std::set
        std::list<std::shared_ptr<TaskPoolWorker>> _idleWorkers; // LIFO
        std::list<std::shared_ptr<Task>> _backlog; // tasks with no thread affinity (assumed async)
        int _maxWorkers {5}; // TODO, from configuration

    public:
        explicit TaskManager(data::Environment & environment) : data::AnchoredWithRoots{environment} {
        }

        std::shared_ptr<data::Anchored> createTask();
        std::shared_ptr<Task> acquireTaskForWorker(TaskThread *worker);
        std::shared_ptr<Task> acquireTaskWhenStealing(TaskThread *worker, const std::shared_ptr<Task> & priorityTask);
        bool allocateNextWorker();
        void queueTask(const std::shared_ptr<Task> &task);
    };
}


