#pragma once

#include <type_traits>

#define REQUIRES(...) typename = std::enable_if_t<__VA_ARGS__>

#define EXISTS(type, expression) std::enable_if_t<std::is_same_v<type, decltype(expression)>>

namespace YQueue::type_traits
{
    /**
     * @brief Determines if the Callable is a stateless functional object that can be invoked with the given arguments.
     */
    template<typename Callable, typename... Ts>
    struct is_stateless_callable : std::bool_constant<
        std::is_invocable_v<Callable, Ts...> &&
        std::is_nothrow_default_constructible_v<Callable> &&
        std::is_empty_v<Callable>
    > {};

    template<typename Callable, typename... Ts>
    constexpr bool is_stateless_callable_v = is_stateless_callable<Callable, Ts...>::value;

    /**
     * @brief Determines if the Callable is a stateless functional object with the given signature.
     */
    template<typename R, typename Callable, typename... Ts>
    struct is_stateless_callable_r : std::bool_constant<
        std::is_invocable_r_v<R, Callable, Ts...> &&
        std::is_nothrow_default_constructible_v<Callable> &&
        std::is_empty_v<Callable>
    > {};

    template<typename R, typename Callable, typename... Ts>
    constexpr bool is_stateless_callable_r_v = is_stateless_callable_r<R, Callable, Ts...>::value;

    /**
     * @brief Determines if the Class has the set_capacity() method.
     */
    template<typename Class, typename = void>
    struct has_set_capacity : std::false_type {};

    template<typename Class>
    struct has_set_capacity<
        Class, std::void_t<
            EXISTS(void, std::declval<Class>().set_capacity(std::declval<typename Class::capacity_type>()))
        >
    > : std::true_type {};

    template<typename T>
    constexpr bool has_set_capacity_v = has_set_capacity<T>::value;
}
