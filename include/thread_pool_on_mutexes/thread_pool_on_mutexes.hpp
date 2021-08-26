#ifndef THREAD_POOL_ON_MUTEXES_H
#define THREAD_POOL_ON_MUTEXES_H

#include <future>
#include <memory>
#include <deque>
#include <functional>
#include <mutex>

// A thread pool implementation that uses only std::mutex for synchronization
// and signaling.
//
// Usage example:
//  thread_pool tp(4, 8);
//  auto future_blob = tp.enqueue([rc] {return rc->read_blob();});
//  auto blob = future_blob.get();
//  tp.shutdown();
//
// This class is thread-safe; any public method can be called from any thread
// in the current process, including the worker threads of the thread pool
// itself.
class thread_pool final
{
public:
    // Initializes this thread pool instance but does not start any threads.
    // No threads will be started until resize() is called with a non-zero
    // 'max_threads' value.
    thread_pool() = default;

    // Initializes this thread pool instance and starts the specified minimum
    // number of worker threads.
    thread_pool(int min_threads, int max_threads)
    {
        resize(min_threads, max_threads);
    }

    // Enqueues a task, which must be a callable object that has no arguments.
    //
    // If there is a vacant thread, it will pick up the task immediately.
    // Otherwise, this method will create a new thread to run the task unless
    // the pool is at the maximum allowed capacity, in which case the task will
    // remain in the queue until an existing thread becomes available.
    //
    // The method returns a future, which will receive the return value of the
    // task.
    template <typename F>
    auto enqueue(F&& f) -> std::future<decltype(f())>
    {
        using PT = std::packaged_task<decltype(f())()>;
        auto pt = std::make_shared<PT>(std::move(f));

        {
            std::lock_guard<std::mutex> lock(global_mutex);
            task_queue.emplace_back([pt] { (*pt)(); });
            wake_up_or_start_thread();
        }

        return pt->get_future();
    }

    // Adjusts the number of worker threads so that it falls within the range
    // defined by the 'min_threads' and 'max_threads' parameters.
    //
    // If the number of currently running tasks in the pool already exceeds
    // 'max_threads', this method returns without directly affecting the
    // threads that run those tasks. Instead, the threads will terminate
    // gradually as their tasks complete until only 'max_threads' are running.
    void resize(int min_threads, int max_threads);

    // Returns the number of currently running threads.
    int thread_count() const
    {
        return current_thread_count;
    }

    // Returns the size of the task queue.
    size_t waiting_tasks() const
    {
        return task_queue.size();
    }

    // Waits for completion of all running tasks and terminates all worker
    // threads. To start the threads again, call resize().
    void shutdown();

    // Calls shutdown().
    ~thread_pool();

private:
    void wake_up_or_start_thread();

    // The queue of tasks to be processed.
    std::deque<std::function<void()>> task_queue;

    struct thread_wrapper;
    friend struct thread_wrapper;

    // The first element in the linked list of threads that are currently
    // processing tasks.
    thread_wrapper* active_threads = nullptr;

    // The first element in the linked list of threads that are dormant
    // (because the task queue is empty).
    thread_wrapper* suspended_threads = nullptr;

    // The first element in the linked list of threads that have exited.
    thread_wrapper* finished_threads = nullptr;

    // The number of currently running threads (the total number of threads
    // in the 'active_threads' and 'suspended_threads' lists combined).
    int current_thread_count = 0;

    // The maximum number of threads that this pool is allowed to have.
    int max_thread_count = 0;

    // The mutex that provides exclusive access to the data members of this
    // thread pool.
    std::mutex global_mutex;

    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;
};

#endif // THREAD_POOL_ON_MUTEXES_H
