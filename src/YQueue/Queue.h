#pragma once

#include <condition_variable>
#include <functional>
#include <optional>

#include <boost/fiber/all.hpp>
#include <boost/noncopyable.hpp>

#include <YQueue/Defines.h>
#include <YQueue/TypeTraits.h>

namespace YQueue
{
    /**
     * @class Queue
     * @brief Simple concurrent queue class.
     * @tparam T value type
     * @tparam Capacity the capacity of the queue
     * @tparam Concurrency the concurrency model to synchronize queue operations
     * @tparam Container the underlying container type
     */
    template<
        typename T,
        typename Container = std::deque<T>,
        Concurrency concurrency = Concurrency::Threads,
        size_t Capacity = YQUEUE_QUEUE_CAPACITY,
        REQUIRES(std::is_copy_constructible_v<T>)
    >
    class Queue final : private boost::noncopyable
    {
        using Mutex = std::conditional_t<concurrency == Concurrency::Threads,
            std::mutex, boost::fibers::mutex
        >;

        using ConditionVariable = std::conditional_t<concurrency == Concurrency::Threads,
            std::condition_variable, boost::fibers::condition_variable
        >;

    public:
        Queue()
            : waiting_(false)
        {
            if constexpr (type_traits::has_set_capacity_v<Container>) {
                container_.set_capacity(Capacity);
            }
        }

        /**
         * @brief Invokes the given callable to consume all available values from the queue.
         * @param callable the consumer callable
         * @return number of dequeued values
         */
        template<typename Callable, REQUIRES(std::is_invocable_v<Callable, T>)>
        size_t consumeAll(Callable&& callable)
        {
            std::vector<T> dequeued;

            {
                std::unique_lock lock(mutex_);

                if (waiting_) {
                    cv_.wait(lock, [this] { return !isEmpty() || !waiting_; });
                }

                const size_t size = container_.size();
                dequeued.reserve(size);

                if constexpr (std::is_nothrow_move_constructible_v<T>) {
                    std::move(container_.begin(), container_.end(), std::back_inserter(dequeued));
                } else {
                    std::copy(container_.begin(), container_.end(), std::back_inserter(dequeued));
                }

                container_.clear();
            }

            std::lock_guard lock(mutex_);

            std::for_each(dequeued.cbegin(), dequeued.cend(), std::forward<Callable>(callable));

            return dequeued.size();
        }


        /**
         * @brief Invokes the given callable to consume one available value from the queue.
         * @param callable the consumer callable
         * @return true if the value was dequeued successfully, false - otherwise
         */
        template<typename Callable, REQUIRES(std::is_invocable_v<Callable, T>)>
        bool consumeOne(Callable&& callable)
        {
            std::optional<T> dequeued;

            {
                std::unique_lock lock(mutex_);

                if (waiting_) {
                    cv_.wait(lock, [this] { return !isEmpty() || !waiting_; });
                }

                if (!isEmpty()) {
                     dequeued = pop();
                }
            }

            if (!dequeued.has_value()) {
                return false;
            }

            std::lock_guard lock(mutex_);

            std::invoke(std::forward<Callable>(callable), dequeued.value());

            return true;
        }

        /**
         * @brief Dequeues a value from the queue.
         * @return a valued optional if the value was dequeued successfully, std::nullopt - otherwise
         */
        std::optional<T> dequeue()
        {
            std::lock_guard lock(mutex_);
            return isEmpty() ? std::nullopt : std::optional(pop());
        }

        /**
         * @brief Disables waiting for enqueued values for all consumers.
         */
        void disableWaiting()
        {
            std::lock_guard lock(mutex_);
            waiting_ = false;
            cv_.notify_all();
        }

        /**
         * @brief Enables waiting for enqueued values in consume methods.
         */
        void enableWaiting()
        {
            std::lock_guard lock(mutex_);
            waiting_ = true;
        }

        /**
         * @brief Enqueues the given value into the queue.
         * @param value the value
         * @return true if the value was enqueued successfully, false if the queue is full
         */
        bool enqueue(T value)
        {
            std::lock_guard lock(mutex_);

            if (!isFull()) {
                container_.push_back(std::move(value));

                if (waiting_) {
                    cv_.notify_one();
                }

                return true;
            }

            return false;
        }

    private:
        bool isEmpty() const
        {
            return container_.empty();
        }

        bool isFull() const
        {
            return container_.size() == Capacity;
        }

        T pop()
        {
            T value = std::move_if_noexcept(container_.front());
            container_.pop_front();

            return value;
        }

    private:
        bool waiting_;
        Container container_;
        Mutex mutex_;
        ConditionVariable cv_;
    };
}
