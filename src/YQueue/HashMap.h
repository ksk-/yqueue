#pragma once

#include <algorithm>
#include <forward_list>
#include <optional>
#include <shared_mutex>
#include <thread>
#include <vector>

#include <boost/noncopyable.hpp>

#include <YQueue/TypeTraits.h>

namespace YQueue
{
    /**
     * @class HashMap
     * @brief Simple thread-safe hashmap class.
     * @tparam Key key type
     * @tparam Value value type
     * @tparam Hash hash-function
     * @note The Hash should be a stateless functional object.
     * @tparam KeyEqual equality comparison predicate
     * @note The KeyEqual callable should be a stateless functional object.
     */
    template<
        typename Key, typename Value, typename Hash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>,
        REQUIRES(std::is_copy_constructible_v<Key>),
        REQUIRES(std::is_copy_constructible_v<Value>),
        REQUIRES(type_traits::is_stateless_callable_r_v<size_t, Hash, Key>),
        REQUIRES(type_traits::is_stateless_callable_r_v<bool, KeyEqual, Key, Key>)
    >
    class HashMap final : private boost::noncopyable
    {
        class Bucket final : private boost::noncopyable
        {
        public:
            std::optional<Value> find(const Key& key) const
            {
                std::shared_lock lock(mutex_);

                const auto it = findValue(key);

                return (it != data_.cend()) ? std::optional(it->second) : std::nullopt;
            }

            std::pair<Value, bool> getOrInsert(const Key& key, Value value)
            {
                std::lock_guard lock(mutex_);

                auto it = findValue(key);

                if (it == data_.end()) {
                    data_.emplace_front(key, std::move(value));
                    return { data_.begin()->second, true };
                }

                return { it->second, false };
            }

            void remove(const Key& key)
            {
                std::lock_guard lock(mutex_);

                data_.remove_if([&key](const auto& pair) {
                    return std::invoke(KeyEqual(), pair.first, key);
                });
            }

        private:
            auto findValue(const Key& key) const
            {
                return std::find_if(data_.begin(), data_.end(), [&key](const auto& pair) {
                    return std::invoke(KeyEqual(), pair.first, key);
                });
            }

        private:
            mutable std::shared_mutex mutex_;
            std::forward_list<std::pair<Key, Value>> data_;
        };

    public:
        /**
         * @brief Constructor.
         * @param bucketCount the number of the buckets containing the data of the map
         * @warning The bucketCount is fixed and there aren't rehashing.
         */
        explicit HashMap(size_t bucketCount = YQUEUE_HASHMAP_BUCKETS)
            : buckets_(bucketCount > 0 ? bucketCount : std::thread::hardware_concurrency())
        {}

        /**
         * @brief Returns the requested value or inserts it if the value isn't found.
         * @param key the key
         * @param value the inserted value
         * @return [value, true] if the value was inserted or [value, false] if it was found in the map
         * @warning The return value is a copy of the value in the map.
         */
        std::pair<Value, bool> getOrInsert(const Key& key, Value value)
        {
            return bucket(key).getOrInsert(key, std::move(value));
        }

        /**
         * @brief Looks for a value by the given key.
         * @param key the key
         * @return value if it exists or std::nullopt otherwise
         * @warning The return value is a copy of the value in the map.
         */
        std::optional<Value> find(const Key& key) const
        {
            return bucket(key).find(key);
        }

        /**
         * @brief Removes the value if the given key exists.
         */
        void remove(const Key& key)
        {
            bucket(key).remove(key);
        }

    private:
        Bucket& bucket(const Key& key)
        {
            return buckets_[std::invoke(Hash(), key) % buckets_.size()];;
        }

        const Bucket& bucket(const Key& key) const
        {
            return buckets_[std::invoke(Hash(), key) % buckets_.size()];
        }

    private:
        std::vector<Bucket> buckets_;
    };
}
