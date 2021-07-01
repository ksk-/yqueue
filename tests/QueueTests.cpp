#include <boost/test/parameterized_test.hpp>

#include "Testing.h"

#include "Queue.h"

void testEnqueueAndDeque()
{
    static const std::vector testValues = { "one", "two", "three", "four", "five" };

    YQueue::Queue<std::string> queue;
    BOOST_TEST(!queue.dequeue().has_value());

    std::for_each(testValues.cbegin(), testValues.cend(), [&queue](const std::string& value) {
        BOOST_CHECK(queue.enqueue(value));
    });

    std::for_each(testValues.cbegin(), testValues.cend(), [&queue](const std::string& value) {
        BOOST_CHECK_EQUAL(value, queue.dequeue().value());
    });

    BOOST_TEST(!queue.dequeue().has_value());
}

void testEnqueueBeforeTheQueueIsFull()
{
    constexpr size_t capacity = 5;

    YQueue::Queue<int, std::deque<int>, YQueue::Concurrency::Threads, capacity> queue;

    for (int i = 0; i < capacity; ++i) {
        BOOST_CHECK(queue.enqueue(i));
    }

    BOOST_CHECK(!queue.enqueue(42));

    queue.dequeue();
    BOOST_CHECK(queue.enqueue(42));
}

template<YQueue::Concurrency Concurrency>
void testConsumeOne(size_t testThreadCount)
{
    constexpr size_t testValueCount = YQUEUE_QUEUE_CAPACITY;

    std::vector<std::string> produced(testThreadCount * testValueCount);
    std::multiset<std::string> consumed;
    std::mutex mutex;

    YQueue::Queue<std::string, std::deque<std::string>, Concurrency, testValueCount> queue;
    queue.enableWaiting();

    Testing::ThreadSafetyTest()
        .runAction(testThreadCount, [&produced, &queue](size_t index) {
            for (size_t i = 0; i < testValueCount; ++i) {
                auto value = std::to_string(index * testValueCount + i);
                produced[index * testValueCount + i] = value;

                while (!queue.enqueue(value)) {};
            }
        })
        .runAction(testThreadCount, [&queue, &consumed, &mutex](size_t index) {
            for (size_t i = 0; i < testValueCount; ++i) {
                const bool isConsumed = queue.consumeOne([&](std::string value) {
                    std::lock_guard lock(mutex);
                    consumed.emplace(std::move(value));
                });

                BOOST_CHECK(isConsumed);
            }
        })
        .wait();

    std::sort(produced.begin(), produced.end());

    Testing::checkEqualCollections(produced, consumed);
}

template<YQueue::Concurrency Concurrency>
void testConsumeAll(size_t testThreadCount)
{
    constexpr size_t testValueCount = YQUEUE_QUEUE_CAPACITY;

    std::vector<std::string> produced(testThreadCount * testValueCount);
    std::multiset<std::string> consumed;

    YQueue::Queue<std::string, std::deque<std::string>, Concurrency> queue;

    Testing::ThreadSafetyTest()
        .runAction(testThreadCount, [&produced, &queue](size_t index) {
            for (size_t i = 0; i < testValueCount; ++i) {
                auto value = std::to_string(index * testValueCount + i);
                produced[index * testValueCount + i] = value;

                while (!queue.enqueue(value)) {};
            }
        })
        .runAction(1, [&consumed, &queue, testThreadCount]([[maybe_unused]] size_t index) {
            for (size_t count = 0; count < testThreadCount * testValueCount;) {
                count += queue.consumeAll([&](std::string value) { consumed.emplace(std::move(value)); });
            };
        })
        .wait();

    std::sort(produced.begin(), produced.end());

    Testing::checkEqualCollections(produced, consumed);
}

boost::unit_test::test_suite* init_unit_test_suite(int, char**)
{
    constexpr std::array threadCount = { 1, 2, 4, 8, 16, 32, 64 };

    boost::unit_test::test_suite* test = BOOST_TEST_SUITE(TEST_SUITE);

    test->add(BOOST_TEST_CASE(&testEnqueueAndDeque));
    test->add(BOOST_TEST_CASE(&testEnqueueBeforeTheQueueIsFull));

    test->add(BOOST_PARAM_TEST_CASE(&testConsumeOne<YQueue::Concurrency::Threads>, threadCount.cbegin(), threadCount.cend()));
    test->add(BOOST_PARAM_TEST_CASE(&testConsumeOne<YQueue::Concurrency::Fibers>, threadCount.cbegin(), threadCount.cend()));

    test->add(BOOST_PARAM_TEST_CASE(&testConsumeAll<YQueue::Concurrency::Threads>, threadCount.cbegin(), threadCount.cend()));
    test->add(BOOST_PARAM_TEST_CASE(&testConsumeAll<YQueue::Concurrency::Fibers>, threadCount.cbegin(), threadCount.cend()));

    return test;
}
