#pragma once

#include <algorithm>
#include <vector>

#include <YQueue/TypeTraits.h>

namespace YQueue::Utils
{
    /**
     * @brief Checks if the map contains an element with the given key.
     * @tparam Map the map type
     * @tparam Key the key type
     */
    template<typename Map, typename Key, REQUIRES(std::is_convertible_v<Key, typename Map::key_type>)>
    bool contains(const Map& map, const Key& key)
    {
        return map.find(key) != map.cend();
    }

    /**
     * @brief Splits the sequence into chunks.
     * @warning All elements of the sequence will be copied into chunks.
     * @tparam InputIterator sequence iterator type
     * @param first an iterator that points to the beginning of the sequence
     * @param last an iterator pointing to the end of the sequence
     * @param count number of chunks
     * @return vector of the chunks
     */
    template<typename InputIterator>
    auto splitToChunks(InputIterator first, InputIterator last, size_t count)
    {
        using value_type = typename std::iterator_traits<InputIterator>::value_type;

        std::vector<std::vector<value_type>> chunks(count);

        std::for_each(first, last, [&chunks, count, index = 0](const value_type& value) mutable {
            chunks[index % count].emplace_back(value);
            ++index;
        });

        return chunks;
    }
}
