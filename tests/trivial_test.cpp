#include <thread_pool/thread_pool.hpp>

#include "catch.hpp"

#include <chrono>
#include <cstring>

#include <iostream>

static void sleep_10ms()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

TEST_CASE("Immediate shutdown")
{
    thread_pool tp(1, 2);

    tp.enqueue(sleep_10ms);
    tp.shutdown();

    REQUIRE(tp.thread_count() == 0);
}

TEST_CASE("Delayed start")
{
    thread_pool tp;

    auto future1 = tp.enqueue(sleep_10ms);
    auto future2 = tp.enqueue(sleep_10ms);

    REQUIRE(tp.waiting_tasks() == 2);
    REQUIRE(tp.thread_count() == 0);

    tp.resize(0, 2);

    REQUIRE(tp.thread_count() == 2);

    future1.wait();
    future2.wait();
}
