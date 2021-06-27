#pragma once

namespace YQueue
{
    /**
     * @enum Concurrency
     * @brief Defines concurrency models.
     */
    enum class Concurrency : uint8_t {
        Threads = 0,    /*!< thread-based concurrency */
        Fibers = 1,     /*!< fiber-based concurrency (see the Boost.Fiber library) */
    };
}
