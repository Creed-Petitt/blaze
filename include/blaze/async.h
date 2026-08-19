#pragma once

#include <boost/asio/awaitable.hpp>

#include <type_traits>

namespace blaze {

template <typename T = void>
using Async = boost::asio::awaitable<T>;

template<typename T>
struct async_result {
    using type = void;
    static constexpr bool is_async = false;
};

template<typename T>
struct async_result<Async<T>> {
    using type = T;
    static constexpr bool is_async = true;
};

} // namespace blaze
