#pragma once

namespace YQueue
{
    /**
     * @interface IConsumer
     * @brief The interface of queue consumers.
     * @tparam Key queue identifier type (key)
     * @tparam Value consumed values type
     */
    template<typename Key, typename Value>
    struct IConsumer
    {
        virtual ~IConsumer() = default;

        /**
         * @brief Consumes the value from the given queue.
         * @param key the key of the queue
         * @param value the consumed value
         */
        virtual void consume(const Key& key, Value value) = 0;
    };
}
