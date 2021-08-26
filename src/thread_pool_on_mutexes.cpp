#include <thread_pool_on_mutexes/thread_pool_on_mutexes.hpp>

#include <thread>
#include <cassert>

// Element of the linked lists in the thread pool object.
struct thread_pool::thread_wrapper final {
    // Starts a new worker thread.
    thread_wrapper(thread_pool* p) :
        pool(p), thread(&thread_wrapper::thread_proc, this)
    {
        signaling_mutex.lock();

        add_to(&p->active_threads);

        ++p->current_thread_count;
    }

    // The worker thread routine.
    void thread_proc();

    // Wakes this thread up.
    void signal()
    {
        do {
            signaling_mutex.unlock();
            signaling_mutex.lock();
        } while (!signal_received);

        signal_received = false;
    }

    // Suspends this thread until another thread calls the signal() method.
    void wait_for_signal()
    {
        std::lock_guard<std::mutex> lock(signaling_mutex);
        signal_received = true;
    }

    // Detaches this thread from the containing list.
    void unlink()
    {
        if (next != nullptr) {
            next->prev = prev;
        }

        if (prev != nullptr) {
            prev->next = next;
        } else {
            *current_list_head = next;
        }
    }

    // Inserts this thread into the specified list.
    void add_to(thread_wrapper** list_head)
    {
        if (*list_head != nullptr) {
            (next = *list_head)->prev = this;
            prev = nullptr;
        } else {
            next = prev = nullptr;
        }

        *(current_list_head = list_head) = this;
    }

    // Moves this thread from its current list to another list.
    void move_to(thread_wrapper** other_list_head)
    {
        if (current_list_head != other_list_head) {
            unlink();
            add_to(other_list_head);
        }
    }

    // Waits for this thread to finish, removes it from the containing
    // list, and deletes this object.
    void join_and_delete()
    {
        pool->global_mutex.unlock();

        thread.join();

        pool->global_mutex.lock();

        unlink();

        delete this;
    }

    // Joins all threads that are in the list of finished threads.
    static void join_all_finished(thread_pool* p)
    {
        while (p->finished_threads != nullptr) {
            p->finished_threads->join_and_delete();
        }
    }

    // The thread pool object that this thread belongs to.
    thread_pool* pool;

    // The thread object that represents this thread of execution.
    std::thread thread;

    // The list in the pool object that contains this thread.
    thread_wrapper** current_list_head;

    // The previous element in the list that contains this thread.
    thread_wrapper* prev;

    // The next element in the list that contains this thread.
    thread_wrapper* next;

    // Mutex for suspending and resuming this thread.
    std::mutex signaling_mutex;

    // Signal delivery confirmation flag. It helps avoid the race condition
    // when the master thread unlocks and then immediately locks the signaling
    // mutex after the worker thread has registered itself as available, but
    // before it acquired a lock on the signaling mutex.
    bool signal_received = false;
};

void thread_pool::thread_wrapper::thread_proc()
{
    pool->global_mutex.lock();

    while (pool->current_thread_count <= pool->max_thread_count) {
        if (pool->task_queue.empty()) {
            move_to(&pool->suspended_threads);

            pool->global_mutex.unlock();

            wait_for_signal();

            pool->global_mutex.lock();

            move_to(&pool->active_threads);
        } else {
            std::function<void()> task = std::move(pool->task_queue.front());
            pool->task_queue.pop_front();

            pool->global_mutex.unlock();

            // The packaged_task class takes care of the potential exceptions.
            task();

            pool->global_mutex.lock();
        }

        join_all_finished(pool);
    }

    move_to(&pool->finished_threads);

    --pool->current_thread_count;

    pool->global_mutex.unlock();
}

void thread_pool::resize(int min_threads, int max_threads)
{
    assert(min_threads <= max_threads);

    std::lock_guard<std::mutex> lock(global_mutex);

    max_thread_count = max_threads;

    // Check if the number of currently running threads exceeds the requested
    // maximum.
    int delta = current_thread_count - max_threads;
    if (delta > 0) {
        // Wake up suspended threads to reduce the number of threads. When a
        // worker thread wakes up, it checks whether the number of running
        // threads exceeds the maximum, and if that is the case, the thread
        // terminates.
        for (thread_wrapper* t = suspended_threads; t != nullptr; t = t->next) {
            t->signal();

            if (--delta == 0) {
                break;
            }
        }
    } else {
        // Check if the number of running threads is fewer than 'min_threads'
        // or if more threads can be started to accommodate the remaining
        // tasks in the queue.
        delta = current_thread_count + static_cast<int>(task_queue.size());

        if (delta > max_threads) {
            delta = max_threads;
        } else if (delta < min_threads) {
            delta = min_threads;
        }

        delta -= current_thread_count;

        while (--delta >= 0) {
            new thread_wrapper(this);
        }
    }

    thread_wrapper::join_all_finished(this);
}

void thread_pool::shutdown()
{
    std::lock_guard<std::mutex> lock(global_mutex);

    max_thread_count = 0;

    while (suspended_threads != nullptr) {
        suspended_threads->signal();

        suspended_threads->join_and_delete();
    }

    while (active_threads != nullptr) {
        active_threads->join_and_delete();
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

void thread_pool::wake_up_or_start_thread()
{
    if (suspended_threads != nullptr) {
        suspended_threads->signal();
    } else if (current_thread_count < max_thread_count) {
        new thread_wrapper(this);
    }
}
