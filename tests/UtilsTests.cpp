#include <deque>
#include <list>
#include <map>
#include <unordered_map>
#include <vector>

#include <boost/mpl/list.hpp>

#include "Testing.h"

#include "Utils.h"

BOOST_TEST_CASE_TEMPLATE_FUNCTION(testContains, Map)
{
    static const Map map = {
        { "one", 1 },
        { "two", 2 },
        { "three", 3 },
        { "four", 4 },
        { "five", 5 },
    };

    for (auto&& [key, _]: map) {
        BOOST_CHECK(YQueue::Utils::contains(map, key));
    }

    BOOST_CHECK(!YQueue::Utils::contains(map, "not_existent_key"));
}

BOOST_TEST_CASE_TEMPLATE_FUNCTION(testSplitToChunks, Container)
{
    static const Container container = { 1, 2, 3, 4, 5 };

    {
        auto chunks = YQueue::Utils::splitToChunks(container.cbegin(), container.cend(), 1);
        Testing::checkEqualCollections(container, chunks[0]);
    }

    {
        auto chunks = YQueue::Utils::splitToChunks(container.cbegin(), container.cend(), 2);
        BOOST_CHECK_EQUAL(2, chunks.size());

        Testing::checkEqualCollections(std::array { 1, 3, 5 }, chunks[0]);
        Testing::checkEqualCollections(std::array { 2, 4 }, chunks[1]);
    }

    {
        auto chunks = YQueue::Utils::splitToChunks(container.cbegin(), container.cend(), 3);
        BOOST_CHECK_EQUAL(3, chunks.size());

        Testing::checkEqualCollections(std::array { 1, 4 }, chunks[0]);
        Testing::checkEqualCollections(std::array { 2, 5 }, chunks[1]);
        Testing::checkEqualCollections(std::array { 3 }, chunks[2]);
    }

    {
        auto chunks = YQueue::Utils::splitToChunks(container.cbegin(), container.cend(), 4);
        BOOST_CHECK_EQUAL(4, chunks.size());

        Testing::checkEqualCollections(std::array { 1, 5 }, chunks[0]);
        Testing::checkEqualCollections(std::array { 2 }, chunks[1]);
        Testing::checkEqualCollections(std::array { 3 }, chunks[2]);
        Testing::checkEqualCollections(std::array { 4 }, chunks[3]);
    }

    {
        auto chunks = YQueue::Utils::splitToChunks(container.cbegin(), container.cend(), 5);
        BOOST_CHECK_EQUAL(5, chunks.size());

        Testing::checkEqualCollections(std::array { 1 }, chunks[0]);
        Testing::checkEqualCollections(std::array { 2 }, chunks[1]);
        Testing::checkEqualCollections(std::array { 3 }, chunks[2]);
        Testing::checkEqualCollections(std::array { 4 }, chunks[3]);
        Testing::checkEqualCollections(std::array { 5 }, chunks[4]);
    }

    {
        auto chunks = YQueue::Utils::splitToChunks(container.cbegin(), container.cend(), 6);
        BOOST_CHECK_EQUAL(6, chunks.size());

        Testing::checkEqualCollections(std::array { 1 }, chunks[0]);
        Testing::checkEqualCollections(std::array { 2 }, chunks[1]);
        Testing::checkEqualCollections(std::array { 3 }, chunks[2]);
        Testing::checkEqualCollections(std::array { 4 }, chunks[3]);
        Testing::checkEqualCollections(std::array { 5 }, chunks[4]);
        BOOST_CHECK(chunks[5].empty());
    }
}

boost::unit_test::test_suite* init_unit_test_suite(int, char**)
{
    using MapTypes = boost::mpl::list<
        std::map<std::string, int>,
        std::unordered_map<std::string, int>
    >;

    using ContainerTypes = boost::mpl::list<
        std::vector<int>,
        std::list<int>,
        std::deque<int>
    >;

    boost::unit_test::test_suite* test = BOOST_TEST_SUITE(TEST_SUITE);
    test->add(BOOST_TEST_CASE_TEMPLATE(testContains, MapTypes));
    test->add(BOOST_TEST_CASE_TEMPLATE(testSplitToChunks, ContainerTypes));

    return test;
}
