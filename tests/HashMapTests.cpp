#include <boost/test/parameterized_test.hpp>

#include "Testing.h"

#include "HashMap.h"

void testGetOrInsert(size_t testThreadCount)
{
    static const std::map<std::string, int> testValues = {
        { "one", 1 },
        { "two", 2 },
        { "three", 3 },
        { "four", 4 },
        { "five", 5 },
    };

    YQueue::HashMap<std::string, int> hashMap;

    for (auto&& [key, value] : testValues) {
        int insertedValue = 0;
        bool inserted = false;

        std::tie(insertedValue, inserted) = hashMap.getOrInsert(key, value);
        BOOST_CHECK_EQUAL(value, insertedValue);
        BOOST_CHECK(inserted);

        std::tie(insertedValue, inserted) = hashMap.getOrInsert(key, value);
        BOOST_CHECK_EQUAL(value, insertedValue);
        BOOST_CHECK(!inserted);
    }

    std::map<std::string, int> insertedValues;
    std::mutex mutex;

    Testing::ThreadSafetyTest()
        .runAction(testThreadCount, [&hashMap, &insertedValues, &mutex](size_t index) {
            for (auto&& [key, value] : testValues) {
                auto&& [insertedValue, _] = hashMap.getOrInsert(key, value);
                BOOST_CHECK_EQUAL(value, insertedValue);

                std::lock_guard lock(mutex);
                insertedValues.emplace(key, std::move(insertedValue));
            }
        })
        .wait();

    Testing::checkEqualCollections(testValues, insertedValues);
}

void testFindAndRemove(size_t testThreadCount)
{
    static const std::map<std::string, int> testValues = {
        { "one", 1 },
        { "two", 2 },
        { "three", 3 },
        { "four", 4 },
        { "five", 5 },
    };

    YQueue::HashMap<std::string, int> hashMap;

    for (auto&& [key, value] : testValues) {
        BOOST_CHECK(!hashMap.find(key).has_value());

        hashMap.getOrInsert(key, value);
        BOOST_CHECK(hashMap.find(key).has_value());
        BOOST_CHECK_EQUAL(value, hashMap.find(key).value());

        hashMap.remove(key);
        BOOST_CHECK(!hashMap.find(key).has_value());
    }

    // NOTE: Fill the map.
    for (auto&& [key, value] : testValues) {
        auto&& [insertedValue, _] = hashMap.getOrInsert(key, value);
        hashMap.getOrInsert(key, std::move(insertedValue));
    }

    std::map<std::string, int> insertedValues;
    std::map<std::string, int> removedValues;
    std::mutex mutex;

    Testing::ThreadSafetyTest()
        .runAction(testThreadCount, [&hashMap, &insertedValues, &removedValues, &mutex](size_t index) {
            for (auto&& [key, value] : testValues) {
                if (auto opt = hashMap.find(key); opt.has_value()) {
                    std::lock_guard lock(mutex);
                    insertedValues.emplace(key, opt.value());
                } else {
                    std::lock_guard lock(mutex);
                    removedValues.emplace(key, value);
                }
            }
        })
        .runAction(testThreadCount, [&hashMap](size_t index) {
            for (auto&& [key, value] : testValues) {
                if (auto opt = hashMap.find(key); opt.has_value()) {
                    hashMap.remove(key);
                }
            }
        })
        .wait();

    insertedValues.merge(removedValues);
    Testing::checkEqualCollections(testValues, insertedValues);
}

boost::unit_test::test_suite* init_unit_test_suite(int, char**)
{
    constexpr std::array threadCount = { 1, 2, 4, 8, 16, 32, 64 };

    boost::unit_test::test_suite* test = BOOST_TEST_SUITE(TEST_SUITE);
    test->add(BOOST_PARAM_TEST_CASE(&testGetOrInsert, threadCount.cbegin(), threadCount.cend()));
    test->add(BOOST_PARAM_TEST_CASE(&testFindAndRemove, threadCount.cbegin(), threadCount.cend()));

    return test;
}
