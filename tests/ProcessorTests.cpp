#include <boost/test/parameterized_test.hpp>

#include "Testing.h"

#include "FConsumer.h"

#include "Processor.h"

using namespace std::chrono_literals;

void testEnqueueAndDequeue()
{
    static const std::vector testValues = { "one", "two", "three", "four", "five" };

    YQueue::Processor<size_t, std::string> processor;

    Testing::ThreadSafetyTest()
        .runAction(std::thread::hardware_concurrency(), [&processor](size_t index) {
            BOOST_TEST(!processor.dequeue(index).has_value());

            std::for_each(testValues.cbegin(), testValues.cend(), [index, &processor](const auto& value) {
                BOOST_CHECK(processor.enqueue(index, value));
            });

            std::for_each(testValues.cbegin(), testValues.cend(), [index, &processor](const auto& value) {
                const std::optional dequeued = processor.dequeue(index);
                BOOST_CHECK(dequeued.has_value());
                BOOST_CHECK_EQUAL(value, dequeued.value());
            });

            BOOST_TEST(!processor.dequeue(index).has_value());
        })
        .wait();
}

void testFibersConcurrency()
{
    constexpr std::array firstProducerValues = { 1, 2, 3, 4, 5 };
    constexpr std::array secondProducerValues = { 42, 43 };

    constexpr size_t consumedCount = firstProducerValues.size() + secondProducerValues.size();

    std::set<int> consumed;

    YQueue::Processor<int, int> processor(1);
    processor.start();

    Testing::ThreadSafetyTest()
        .runAction(1, [&processor, &firstProducerValues](size_t index) {
            for (int value : firstProducerValues) {
                processor.enqueue(0, value);
                std::this_thread::sleep_for(1ms);
            }
        })
        .runAction(1, [&processor, &secondProducerValues](size_t index) {
            for (int value : secondProducerValues) {
                processor.enqueue(1, value);
            }
        })
        .runAction(1, [&processor, &consumed](size_t index) {
            auto consumer = std::make_shared<YQueue::FConsumer<int, int>>(
                [&consumed](int key, int value) {
                    consumed.emplace(value);
                }
            );

            processor.subscribe(0, consumer);
            processor.subscribe(1, consumer);
        })
        .wait();

    // NOTE: Waiting until all produced values are consumed.
    while (consumed.size() < consumedCount) {
        std::this_thread::sleep_for(1ns);
    }

    processor.stop();

    Testing::checkEqualCollections(std::array { 1, 2, 3, 4, 5, 42, 43 }, consumed);
}

void testConsumeBySubscription(const std::tuple<size_t, size_t, bool>& params)
{
    const size_t testThreadCount = std::get<0>(params);
    const size_t testProcessorTreadCount = std::get<1>(params);
    const bool startImmediately = std::get<2>(params);

    static const std::vector testValues = { "one", "two", "three", "four", "five" };

    std::vector<std::thread> threads;
    threads.reserve(testThreadCount);

    std::vector<std::vector<std::string>> consumed;
    consumed.resize(testThreadCount);

    std::for_each(consumed.begin(), consumed.end(), [](auto& consumed) { consumed.reserve(testValues.size()); });

    YQueue::Processor<std::string, std::string> processor(testProcessorTreadCount);

    if (startImmediately) {
        processor.start();
    }

    Testing::ThreadSafetyTest()
        .runAction(testThreadCount, [&processor](size_t index) {
            std::for_each(testValues.cbegin(), testValues.cend(), [&processor, index](const auto& value) {
                BOOST_CHECK(processor.enqueue("queue_" + std::to_string(index), value));
            });
        })
        .runAction(testThreadCount, [&processor, &consumed](size_t index) {
            auto consumer = std::make_shared<YQueue::FConsumer<std::string, std::string>>(
                [&consumed = consumed[index]](const std::string& key, std::string value) {
                    consumed.emplace_back(std::move(value));
                }
            );

            BOOST_CHECK(processor.subscribe("queue_" + std::to_string(index), consumer));
        })
        .wait();

    if (!startImmediately) {
        processor.start();
    }

    // NOTE: Waiting until all produced values are consumed.
    std::for_each(consumed.cbegin(), consumed.cend(), [](const auto& consumed) {
        while (consumed.size() < testValues.size()) {
            std::this_thread::sleep_for(1ns);
        }
    });

    processor.stop();

    std::for_each(consumed.cbegin(), consumed.cend(), [](const auto& consumed) {
        Testing::checkEqualCollections(testValues, consumed);
    });
}

void testSubscribeAnUnSubscribe(const std::tuple<size_t, size_t, bool>& params)
{
    const size_t testThreadCount = std::get<0>(params);
    const size_t testProcessorTreadCount = std::get<1>(params);
    const bool startImmediately = std::get<2>(params);

    static const std::vector testValues = { "one", "two", "three", "four", "five" };

    std::vector<std::thread> threads;
    threads.reserve(testThreadCount);

    std::vector<std::vector<std::string>> consumed(testThreadCount);
    consumed.resize(testThreadCount);

    std::for_each(consumed.begin(), consumed.end(), [](auto& consumed) { consumed.reserve(testValues.size()); });

    YQueue::Processor<std::string, std::string> processor(testProcessorTreadCount);

    if (startImmediately) {
        processor.start();
    }

    Testing::ThreadSafetyTest()
        .runAction(testThreadCount, [&processor](size_t index) {
            std::for_each(testValues.cbegin(), testValues.cend(), [&processor, index](const auto& value) {
                BOOST_CHECK(processor.enqueue("queue_" + std::to_string(index), value));
            });
        })
        .runAction(testThreadCount, [&processor, &consumed](size_t index) {
            auto consumer = std::make_shared<YQueue::FConsumer<std::string, std::string>>(
                [&consumed = consumed[index], index](const std::string& key, std::string value) {
                    consumed.emplace_back(std::move(value));
                }
            );

            BOOST_CHECK(processor.subscribe("queue_" + std::to_string(index), consumer));
        })
        .runAction(testThreadCount, [&processor, &consumed](size_t index) {
            const std::string queue = "queue_" + std::to_string(index);

            if (processor.unsubscribe(queue)) {
                auto consumer = std::make_shared<YQueue::FConsumer<std::string, std::string>>(
                    [&consumed = consumed[index], index](const std::string& key, std::string value) {
                        consumed.emplace_back(std::move(value));
                    }
                );

                BOOST_CHECK(processor.subscribe(queue, consumer));
                BOOST_CHECK(!processor.subscribe(queue, consumer));
            }
        })
        .wait();

    if (!startImmediately) {
        processor.start();
    }

    // NOTE: Waiting until all produced values are consumed.
    std::for_each(consumed.cbegin(), consumed.cend(), [](const auto& consumed) {
        while (consumed.size() < testValues.size()) {
            std::this_thread::sleep_for(1ns);
        }
    });

    processor.stop();

    std::for_each(consumed.cbegin(), consumed.cend(), [](const auto& consumed) {
        Testing::checkEqualCollections(testValues, consumed);
    });
}

boost::unit_test::test_suite* init_unit_test_suite(int, char**)
{
    constexpr std::array params = {
        std::tuple(1, 1, false),
        std::tuple(1, 4, false),
        std::tuple(2, 4, false),
        std::tuple(4, 4, false),
        std::tuple(4, 2, false),
        std::tuple(4, 1, false),
        std::tuple(111, 11, false),
        std::tuple(11, 111, false),

        std::tuple(1, 1, true),
        std::tuple(1, 4, true),
        std::tuple(2, 4, true),
        std::tuple(4, 4, true),
        std::tuple(4, 2, true),
        std::tuple(4, 1, true),
        std::tuple(111, 11, true),
        std::tuple(11, 111, true),
    };

    boost::unit_test::test_suite* test = BOOST_TEST_SUITE(TEST_SUITE);
    test->add(BOOST_TEST_CASE(&testEnqueueAndDequeue));
    test->add(BOOST_TEST_CASE(&testFibersConcurrency));
    test->add(BOOST_PARAM_TEST_CASE(&testConsumeBySubscription, params.cbegin(), params.cend()));
    test->add(BOOST_PARAM_TEST_CASE(&testSubscribeAnUnSubscribe, params.cbegin(), params.cend()));

    return test;
}
