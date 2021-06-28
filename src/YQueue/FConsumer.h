#pragma once

#include <functional>

#include <YQueue/IConsumer.h>
#include <YQueue/TypeTraits.h>

namespace YQueue
{
    /**
     * @class FConsumer
     * @brief Functional implementation of IConsumer.
     */
    template<typename Key, typename Value>
    class FConsumer final : public IConsumer<Key, Value>
    {
    public:
        /**
         * @brief Constructor.
         * @param callable the consumer callable
         */
        template<typename Callable, REQUIRES(std::is_invocable_v<Callable, Key, Value>)>
        FConsumer(Callable&& callable)
            : callable_(std::forward<Callable>(callable))
        {}

        void consume(const Key& key, Value value) override
        {
            std::invoke(callable_, key, std::move(value));
        }

    private:
        std::function<void(const Key&, Value)> callable_;
    };
}
