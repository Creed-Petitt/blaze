#ifndef BLAZE_WRAPPERS_H
#define BLAZE_WRAPPERS_H

#include <utility>
#include <ostream>
#include <type_traits>
#include <boost/json.hpp>

namespace blaze {

/**
 * @brief Base wrapper providing common operators to avoid explicit dereferencing.
 */
template<typename T, typename Derived>
struct Wrapper {
    T value;
    
    Wrapper() = default;
    Wrapper(T v) : value(std::move(v)) {}

    T& operator*() { return value; }
    const T& operator*() const { return value; }
    T* operator->() { return &value; }
    const T* operator->() const { return &value; }

    // Standard operators to allow dereference-free usage
    template<typename U> bool operator==(const U& other) const { return value == other; }
    template<typename U> bool operator!=(const U& other) const { return value != other; }
    template<typename U> auto operator+(const U& other) const { return value + other; }
    template<typename U> auto operator-(const U& other) const { return value - other; }
    
    // Support for string concatenation and other common operations
    template<typename U> Derived& operator+=(const U& other) { value += other; return static_cast<Derived&>(*this); }

    friend std::ostream& operator<<(std::ostream& os, const Derived& w) {
        return os << w.value;
    }

    // String View Conversion (Solves 'std::string' double-conversion issue)
    template <typename U = T>
    operator std::enable_if_t<
        std::is_convertible<U, boost::json::string_view>::value, 
        boost::json::string_view
    >() const {
        return boost::json::string_view(value);
    }

    // Enabled ONLY if T is NOT constructible (like int) and NOT a string view (handled above).
    template <typename U = T>
    operator std::enable_if_t<
        !std::is_constructible<boost::json::value, U>::value && 
        !std::is_convertible<U, boost::json::string_view>::value,
        boost::json::value
    >() const {
        return boost::json::value_from(value);
    }

    // Handles int, bool, and normal usage like User u = wrapper
    operator T&() { return value; }
    operator const T&() const { return value; }
};

template<typename T>
struct Path : Wrapper<T, Path<T>> {
    using value_type = T;
    using Wrapper<T, Path<T>>::Wrapper;
    
    // Explicitly handle constructor ambiguity
    Path(T v) : Wrapper<T, Path<T>>(std::move(v)) {}
    Path() = default;
};

template<typename T>
struct Body : Wrapper<T, Body<T>> {
    using value_type = T;
    using Wrapper<T, Body<T>>::Wrapper;
    
    Body(T v) : Wrapper<T, Body<T>>(std::move(v)) {}
    Body() = default;
};

template<typename T>
struct Query : Wrapper<T, Query<T>> {
    using value_type = T;
    using Wrapper<T, Query<T>>::Wrapper;
    
    Query(T v) : Wrapper<T, Query<T>>(std::move(v)) {}
    Query() = default;
};

template<typename T>
struct Context : Wrapper<T, Context<T>> {
    using value_type = T;
    using Wrapper<T, Context<T>>::Wrapper;
    
    Context(T v) : Wrapper<T, Context<T>>(std::move(v)) {}
    Context() = default;
};

} // namespace blaze

#endif
