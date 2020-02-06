thread_pool_on_mutexes
======================

This thread pool implementation uses C++11 synchronization primitives and
provides a clean and concise interface.

Interface
---------

 1. Initialization

        #include <thread_pool_on_mutexes/thread_pool_on_mutexes.hpp>

        int min_threads = 2;
        int max_threads = 8;

        thread_pool tp(min_threads, max_threads);

 2. Scheduling a task

    In this example, a job is scheduled and a two-second timeout is handled.

        auto future_result = tp.enqueue([job] { return job->compute(); });

        using namespace std::chrono_literals;

        if (future_result.wait_for(2s) == std::future_status::ready) {
            // The job is done within two seconds. Get the result.
            const auto result = future_result.get();

        } else {
            // The job is still running. Handle the timeout.

        }

 3. Scaling the pool up or down

        // Bump the maximum number of worker threads to 16.
        max_threads = 16;
        tp.resize(min_threads, max_threads);

 4. Termination

        // Discard the queue, wait until the remaining running tasks
        // finish, and terminate the worker threads.
        tp.shutdown();

How to build
------------

Because of its minuscule size, this project is meant to be vendored and
integrated into an existing build system. A `CMakeLists.txt` file is
provided for convenience.
