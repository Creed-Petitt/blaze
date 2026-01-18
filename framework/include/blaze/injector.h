#ifndef BLAZE_INJECTOR_H
#define BLAZE_INJECTOR_H

#include <blaze/di.h>
#include <blaze/request.h>
#include <blaze/response.h>
#include <tuple>
#include <functional>
#include <type_traits>
#include <memory>

namespace blaze {

template <typename T>
struct function_traits : public function_traits<decltype(&T::operator())> {};

template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...) const> {
    using args_tuple = std::tuple<Args...>;
};

template <typename ReturnType, typename... Args>
struct function_traits<ReturnType(*)(Args...)> {
    using args_tuple = std::tuple<Args...>;
};

template<typename T>
struct is_shared_ptr : std::false_type {};

template<typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};

template<typename T>
inline constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;


template<typename Func, typename Tuple, size_t... Is>
auto call_with_deps_impl(Func& func, ServiceProvider& provider, Request& req, Response& res, std::index_sequence<Is...>) {
    
    // Resolve all dependencies into a tuple of shared_ptrs.
    auto deps = std::make_tuple(
        [&]() -> std::any {
            using ArgType = std::tuple_element_t<Is, Tuple>;
            using PureType = std::remove_cvref_t<ArgType>;
            
            if constexpr (std::is_same_v<PureType, Request> || std::is_same_v<PureType, Response>) {
                return nullptr; // Placeholders
            } else if constexpr (is_shared_ptr_v<PureType>) {
                return provider.resolve<typename PureType::element_type>();
            } else {
                return provider.resolve<PureType>();
            }
        }()...
    );

    // Call the function, dereferencing the pointers as needed.
    return func(
        [&]() -> decltype(auto) {
            using ArgType = std::tuple_element_t<Is, Tuple>;
            using PureType = std::remove_cvref_t<ArgType>;
            
            if constexpr (std::is_same_v<PureType, Request>) {
                return req;
            } else if constexpr (std::is_same_v<PureType, Response>) {
                return res;
            } else {
                auto val = std::get<Is>(deps);
                if constexpr (is_shared_ptr_v<ArgType>) {
                    return std::any_cast<ArgType>(val);
                } else {
                    // User asked for T&, dereference the shared_ptr<T>
                    using TargetType = std::shared_ptr<PureType>;
                    return *std::any_cast<TargetType>(val);
                }
            }
        }()... 
    );
}

template<typename Func>
auto inject_and_call(Func& func, ServiceProvider& provider, Request& req, Response& res) {
    using Traits = function_traits<Func>;
    using ArgsTuple = typename Traits::args_tuple;
    
    return call_with_deps_impl<Func, ArgsTuple>(
        func, 
        provider, 
        req, 
        res, 
        std::make_index_sequence<std::tuple_size_v<ArgsTuple>>{}
    );
}

} // namespace blaze

#endif
