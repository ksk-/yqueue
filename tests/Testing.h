#pragma once

#include <thread>

#include <boost/noncopyable.hpp>
#include <boost/test/included/unit_test.hpp>

// NOTE: This is a requirement of Boost.Test.
namespace std
{
    template<typename Key, typename Value>
    inline std::ostream& operator<<(std::ostream& out, const std::pair<Key, Value>& pair)
    {
        return out << "[ " << pair.first << " , " << pair.second << " ]";
    }
}

namespace Testing
{
    using Action = std::function<void(size_t)>;

    class ThreadSafetyTest final : private boost::noncopyable
    {
    public:
        ThreadSafetyTest& runAction(size_t threadCount, Action action)
        {
            for (size_t index = 0; index < threadCount; ++index) {
                threads_.emplace_back(action, index);
            }

            return *this;
        }

        void wait()
        {
            std::for_each(threads_.begin(), threads_.end(), std::mem_fn(&std::thread::join));
        }

    private:
        std::vector<std::thread> threads_;
    };

    template<bool expression>
    void checkExpression()
    {
        BOOST_CHECK(expression);
    }

    template<typename C1, typename C2>
    void checkEqualCollections(const C1& expected, const C2& actual)
    {
        BOOST_CHECK_EQUAL_COLLECTIONS(
            std::cbegin(expected), std::cend(expected),
            std::cbegin(actual), std::cend(actual)
        );
    }
}
