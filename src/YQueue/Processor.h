#pragma once

#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/fiber/all.hpp>
#include <boost/noncopyable.hpp>
#include <boost/signals2.hpp>

#include <YQueue/HashMap.h>
#include <YQueue/IConsumer.h>
#include <YQueue/TypeTraits.h>
#include <YQueue/Queue.h>
#include <YQueue/Utils.h>

namespace YQueue
{
    template<
        typename Key, typename Value,
        typename QueueType = Queue<Value, boost::circular_buffer<Value>, Concurrency::Fibers>,
        REQUIRES(std::is_copy_constructible_v<Key>),
        REQUIRES(std::is_copy_constructible_v<Value>)
    >
    /**
     * @class Processor
     * @brief The queues processor
     * @tparam Key queue identifier type (key)
     * @tparam Value queued values type
     * @tparam Capacity the capacity of the queues
     */
    class Processor final : boost::noncopyable
    {
    public:
        /**
         * @brief Constructor.
         * @param threadCount number of processing threads
         * @note If the threadCount equal to 0 will be use thr number of concurrent threads supported on the platform.
         */
        explicit Processor(size_t threadCount = YQUEUE_PROCESSOR_THREADS)
            : isRunning_(false)
            , maxThreadCount_(threadCount > 0 ? threadCount : std::thread::hardware_concurrency())
        {}

        /**
         * @brief Destructor.
         * @note The processing will be stopped before the destruction.
         */
        ~Processor()
        {
            stop();
        }

        /**
         * @brief Dequeues a value from the given queue.
         * @param key the key of the queue
         * @note The queue will be created if it isn't exists.
         * @return a valued optional if the value was dequeued successfully, std::nullopt - otherwise
         */
        std::optional<Value> dequeue(const Key& key)
        {
            auto queue = queues_.find(key);
            return queue.has_value() ? queue.value()->dequeue() : std::nullopt;
        }

        /**
         * @brief Enqueues the given value into the given queue.
         * @param key the key of the queue
         * @param value the value
         * @note The queue will be created if it isn't exists.
         * @return true if the value was enqueued successfully, false - otherwise
         */
        bool enqueue(const Key& key, Value value)
        {
            auto&& [queue, _] = queues_.getOrInsert(key, std::make_shared<QueueType>());
            return queue->enqueue(std::move(value));
        }

        /**
         * @brief Starts the processing.
         */
        void start()
        {
            std::lock_guard lock(mutex_);

            if (!isRunning_) {
                isRunning_ = true;

                const size_t threadCount = std::min(maxThreadCount_, consumers_.size());
                threadPool_.emplace(threadCount);

                auto consumers = Utils::splitToChunks(consumers_.cbegin(), consumers_.cend(), threadCount);

                for (size_t i = 0; i < threadCount; ++i) {
                    boost::asio::post(threadPool_.value(), [this, consumers = std::move(consumers[i])] {
                        runConsumers(consumers);
                    });
                }
            }
        }

        /**
         * @brief Stops the processing.
         */
        void stop()
        {
            std::lock_guard lock(mutex_);

            if (isRunning_) {
                isRunning_ = false;
                std::invoke(onStopped_);

                threadPool_->wait();
                threadPool_.reset();
            }
        }

        /**
         * @brief Subscribes the given consumer to the given queue.
         * @param key the key of the queue
         * @param consumer the consumer
         * @note The queue will be created if it isn't exists.
         * @warning Only one consumer can be subscribed to the queue.
         * @return true if the consumer was subscribed successfully, false - otherwise
         */
        bool subscribe(const Key& key, std::shared_ptr<IConsumer<Key, Value>> consumer)
        {
            bool inserted = false;

            std::lock_guard lock(mutex_);

            if (isRunning_ && !Utils::contains(consumers_, key)) {
                stop();
                std::tie(std::ignore, inserted) = consumers_.try_emplace(key, std::move(consumer));
                start();
            } else {
                std::tie(std::ignore, inserted) = consumers_.try_emplace(key, std::move(consumer));
            }

            if (inserted) {
                auto&& [queue, _] = queues_.getOrInsert(key, std::make_shared<QueueType>());
                onStopped_.connect([queue] { queue->disableWaiting(); });
                queue->enableWaiting();
            }

            return inserted;
        }

        /**
         * @brief Unsubscribes the consumer from the given queue.
         * @param key the key of the queue
         * @return true if the consumer was unsubscribed successfully, false if there wasn't any subscriber
         */
        bool unsubscribe(const Key& key)
        {
            bool removed = false;

            std::lock_guard lock(mutex_);

            stop();
            removed = consumers_.erase(key) > 0;
            start();

            return removed;
        }

    private:
        template<typename Consumers>
        void runConsumers(const Consumers& consumers)
        {
            std::vector<boost::fibers::fiber> fibers;

            for (auto&& [key, consumer] : consumers) {
                fibers.emplace_back([&key, consumer, this] {
                    auto&& [queue, _] = queues_.getOrInsert(key, std::make_shared<QueueType>());

                    while (isRunning_) {
                        boost::this_fiber::yield();
                        queue->consumeAll([&](Value value) { consumer->consume(key, std::move(value)); });
                    }
                });
            }

            std::for_each(fibers.begin(), fibers.end(), std::mem_fn(&boost::fibers::fiber::join));
        }

    private:
        bool isRunning_;
        boost::signals2::signal<void()> onStopped_;

        size_t maxThreadCount_;
        std::optional<boost::asio::thread_pool> threadPool_;

        HashMap<Key, std::shared_ptr<QueueType>> queues_;

        std::recursive_mutex mutex_;
        std::unordered_map<Key, std::shared_ptr<IConsumer<Key, Value>>> consumers_;
    };
}
