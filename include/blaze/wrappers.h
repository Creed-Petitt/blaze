#ifndef BLAZE_WRAPPERS_H
#define BLAZE_WRAPPERS_H

#include <utility>
#include <ostream>
#include <type_traits>
#include <boost/json.hpp>

namespace blaze {

// Base Wrapper for Non-Class types (int, float)
// Cannot inherit from int, so we wrap it.
template<typename T, typename Derived, typename Enable = void>
struct Wrapper {
    T value;
    
    Wrapper() = default;
    Wrapper(T v) : value(std::move(v)) {}

    // Implicit conversion to T& (Handles math, assignment, JSON)
    operator T&() { return value; }
    operator const T&() const { return value; }

    // Operators for seamless usage
    template<typename U> bool operator==(const U& other) const { return value == other; }
    template<typename U> bool operator!=(const U& other) const { return value != other; }
    template<typename U> auto operator+(const U& other) const { return value + other; }
    template<typename U> auto operator-(const U& other) const { return value - other; }

    friend std::ostream& operator<<(std::ostream& os, const Derived& w) {
        return os << w.value;
    }
};

// Specialized Wrapper for Class types (User, string, vector)
// Inherits from T so user.age works directly.
template<typename T, typename Derived>
struct Wrapper<T, Derived, std::enable_if_t<std::is_class_v<T>>> : public T {
    using T::T; // Inherit constructors
    
    Wrapper() = default;
    Wrapper(T v) : T(std::move(v)) {}

    // Implicit conversion to T& (Identity)
    operator T&() { return *this; }
    operator const T&() const { return *this; }

    friend std::ostream& operator<<(std::ostream& os, const Derived& w) {
        // Assume T is streamable or rely on implicit conversion
        return os << static_cast<const T&>(w);
    }
};

template<typename T>
struct Path : Wrapper<T, Path<T>> {
    using value_type = T;
    using Wrapper<T, Path<T>>::Wrapper;
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
