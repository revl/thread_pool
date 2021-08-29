#include <thread_pool/thread_pool.hpp>

#include <thread>
#include <cassert>

// Each thread_wrapper object represents a single worker thread.
struct thread_pool::thread_wrapper final {
    // Starts a new worker thread.
    thread_wrapper(thread_pool* p) :
        pool(p), thread(&thread_wrapper::thread_proc, this)
    {
        ++p->total_thread_count;
    }

    // The worker thread routine.
    void thread_proc();

    // Joins all threads that are in the list of finished threads and deletes
    // their wrapper objects.
    static void join_all_finished(thread_pool* p)
    {
        thread_wrapper* finished = p->finished_threads;

        while (finished != nullptr) {
            finished->thread.join();

            thread_wrapper* next = finished->next_finished;
            delete finished;
            finished = next;
        }

        p->finished_threads = nullptr;
    }

    // The thread pool object that this thread belongs to.
    thread_pool* pool;

    // The thread object that represents this thread of execution.
    std::thread thread;

    // When this thread finishes, it will insert itself into the
    // finished_threads list of the parent thread pool.
    thread_wrapper* next_finished;
};

void thread_pool::thread_wrapper::thread_proc()
{
    std::unique_lock<std::mutex> lock(pool->global_mutex);

    while (pool->total_thread_count <= pool->max_thread_count) {
        if (pool->task_queue.empty()) {
            ++pool->sleeping_thread_count;

            pool->cv.wait(lock);

            --pool->sleeping_thread_count;
        } else {
            std::function<void()> task = std::move(pool->task_queue.front());
            pool->task_queue.pop_front();

            lock.unlock();

            // The packaged_task class takes care of the potential exceptions.
            task();

            lock.lock();
        }

        join_all_finished(pool);
    }

    next_finished = pool->finished_threads;
    pool->finished_threads = this;

    --pool->total_thread_count;
}

void thread_pool::resize(int min_threads, int max_threads)
{
    assert(min_threads <= max_threads);

    std::unique_lock<std::mutex> lock(global_mutex);

    max_thread_count = max_threads;

    // Check if the number of currently running threads exceeds the requested
    // maximum.
    int delta = total_thread_count - max_threads;
    if (delta > 0) {
        // Wake up suspended threads to reduce the number of threads. When a
        // worker thread wakes up, it checks whether the number of running
        // threads exceeds the maximum, and if that is the case, the thread
        // terminates.
        lock.unlock();
        do {
            cv.notify_one();
        } while (--delta != 0);
        lock.lock();
    } else {
        // Check if the number of running threads is less than 'min_threads'
        // or if more threads can be started to accommodate the remaining
        // tasks in the queue.
        delta = total_thread_count + static_cast<int>(task_queue.size());

        if (delta > max_threads) {
            delta = max_threads;
        } else if (delta < min_threads) {
            delta = min_threads;
        }

        delta -= total_thread_count;

        while (--delta >= 0) {
            start_new_thread();
        }
    }

    thread_wrapper::join_all_finished(this);
}

void thread_pool::shutdown()
{
    std::unique_lock<std::mutex> lock(global_mutex);

    max_thread_count = 0;

    cv.notify_all();
    while (total_thread_count > 0) {
        lock.unlock();
        lock.lock();
    }

    thread_wrapper::join_all_finished(this);
}

thread_pool::~thread_pool()
{
    try {
        shutdown();
    }
    catch (...) {
    }
}

void thread_pool::start_new_thread()
{
    new thread_wrapper(this);
}
