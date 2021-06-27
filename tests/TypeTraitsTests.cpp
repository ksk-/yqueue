#include <boost/circular_buffer.hpp>
#include <boost/circular_buffer/space_optimized.hpp>
#include <boost/mpl/list.hpp>

#include "Testing.h"

#include "TypeTraits.h"

namespace
{
    struct Sum final
    {
        int operator()(int a, int b) const
        {
            return a + b;
        }
    };

    struct Value final
    {
        int value;

        Value(int value = 0)
            : value(value)
        {}

        int operator()()
        {
            return value;
        }
    };

    struct NotConstructible final
    {
        NotConstructible() = delete;
        void operator()() const {}
    };

    int sum(int a, int b)
    {
        return a + b;
    }
}

void testIsStateLessCallable()
{
    using namespace YQueue;

    Testing::checkExpression<type_traits::is_stateless_callable_v<Sum, int, int>>();
    Testing::checkExpression<type_traits::is_stateless_callable_r_v<int, Sum, int, int>>();

    Testing::checkExpression<!type_traits::is_stateless_callable_r_v<int, Sum, int>>();
    Testing::checkExpression<!type_traits::is_stateless_callable_r_v<std::string, Sum, int, int>>();

    Testing::checkExpression<!type_traits::is_stateless_callable_v<Value>>();
    Testing::checkExpression<!type_traits::is_stateless_callable_v<NotConstructible>>();
    Testing::checkExpression<!type_traits::is_stateless_callable_v<decltype(sum), int, int>>();
}

void testHasSetCapacity()
{
    using namespace YQueue;

    boost::mpl::for_each<
        boost::mpl::list<
            boost::circular_buffer<int>,
            boost::circular_buffer_space_optimized<int>
        >
    >([](auto value) { Testing::checkExpression<type_traits::has_set_capacity_v<decltype(value)>>(); });

    boost::mpl::for_each<
        boost::mpl::list<
            std::deque<int>,
            std::list<int>,
            std::vector<int>
        >
    >([](auto value) { Testing::checkExpression<!type_traits::has_set_capacity_v<decltype(value)>>(); });
}

boost::unit_test::test_suite* init_unit_test_suite(int, char**)
{
    boost::unit_test::test_suite* test = BOOST_TEST_SUITE(TEST_SUITE);
    test->add(BOOST_TEST_CASE(&testIsStateLessCallable));
    test->add(BOOST_TEST_CASE(&testHasSetCapacity));

    return test;
}
