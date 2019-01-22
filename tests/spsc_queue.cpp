
#include <atomic>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include <lockfree/spsc_queue.hpp>

#include "gtest/gtest.h"


TEST(spsc_queue_test, push_and_pop)
{
    lockfree::spsc_queue<int> q;
    int out;

    ASSERT_FALSE(q.pop(out));
    ASSERT_TRUE(q.empty());

    q.push(123);
    ASSERT_TRUE(q.pop(out));
    ASSERT_EQ(out, 123);
    ASSERT_FALSE(q.pop(out));
    ASSERT_TRUE(q.empty());

    for (int i = 0; i != 5; ++i) {
        q.push(i);
    }
    for (int i = 0; i != 5; ++i) {
        ASSERT_TRUE(q.pop(out));
        ASSERT_EQ(out, i);
    }
    ASSERT_FALSE(q.pop(out));
    ASSERT_TRUE(q.empty());
}
