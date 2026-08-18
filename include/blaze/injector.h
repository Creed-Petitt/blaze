#pragma once

#include <blaze/di.h>
#include <blaze/exceptions.h>
#include <blaze/request.h>
#include <blaze/response.h>
#include <blaze/wrappers.h>
#include <blaze/traits.h>
#include <blaze/util/string.h>
#include <tuple>
#include <functional>
#include <type_traits>
#include <memory>
#include <charconv>
#include <boost/describe.hpp>
#include <boost/mp11.hpp>

namespace blaze {

template<typename>
inline constexpr bool always_false_v = false;

// Detects if T has a void validate() method
template <typename T, typename = void>
struct has_validate : std::false_type {};

template <typename T>
struct has_validate<T, std::void_t<decltype(std::declval<T>().validate())>> : std::true_type {};

template <typename T>
void try_validate(T& model) {
    if constexpr (has_validate<T>::value) {
        model.validate();
    }
}

namespace detail {

    // Resolve a single argument into a std::any (shared_ptr<T>)
    template<typename ArgType, size_t Is, typename Tuple>
    std::any resolve_arg(Request& req, Response& res) {
        using PureType = std::remove_cvref_t<ArgType>;
        
        if constexpr (std::is_same_v<PureType, Request> || std::is_same_v<PureType, Response>) {
            return nullptr; // Placeholders
        } else if constexpr (is_instantiation_of<Path, PureType>::value) {
            using InnerT = PureType::value_type;
            static constexpr size_t idx = count_instances_before<Path, Is, Tuple>::count();
            if (idx < req.path_values.size()) {
                return std::make_shared<PureType>(convert_string<InnerT>(req.path_values[idx]));
            }
            return std::make_shared<PureType>();
        } else if constexpr (is_instantiation_of<Body, PureType>::value) {
            using InnerT = PureType::value_type;
            auto model = req.json<InnerT>();
            try_validate(model);
            return std::make_shared<PureType>(std::move(model));
        } else if constexpr (is_instantiation_of<Query, PureType>::value) {
            using InnerT = PureType::value_type;
            InnerT model{};
            using Members = boost::describe::describe_members<InnerT, boost::describe::mod_any_access>;
            boost::mp11::mp_for_each<Members>([&](auto meta) {
                const std::string key = meta.name;
                if (req.query.contains(key)) {
                    using FieldT = std::remove_cvref_t<decltype(model.*meta.pointer)>;
                    model.*meta.pointer = convert_string<FieldT>(req.query.at(key));
                }
            });
            try_validate(model);
            return std::make_shared<PureType>(model);
        } else if constexpr (is_instantiation_of<Context, PureType>::value) {
            using InnerT = PureType::value_type;
            auto val = req.get_opt<InnerT>(typeid(InnerT).name());
            if (val) return std::make_shared<PureType>(*val);
            return std::make_shared<PureType>();
        } else {
            if constexpr (boost::describe::has_describe_members<PureType>::value) {
                auto model = req.json<PureType>();
                try_validate(model);
                return std::make_shared<PureType>(std::move(model));
            } else {
                static_assert(always_false_v<PureType>,
                    "Unsupported handler argument. Use Request&, Response&, Path<T>, Query<T>, Body<T>, Context<T>, or req.service<T>().");
            }
        }
    }

    // Unwrap the std::any into the exact type the function needs
    template<typename ArgType>
    decltype(auto) unwrap_arg(std::any& val, Request& req, Response& res) {
        using PureType = std::remove_cvref_t<ArgType>;
        
        if constexpr (std::is_same_v<PureType, Request>) {
            return req;
        } else if constexpr (std::is_same_v<PureType, Response>) {
            return res;
        } else {
            if constexpr (is_shared_ptr_v<ArgType>) {
                return std::any_cast<ArgType>(val);
            } else {
                using TargetType = std::shared_ptr<PureType>;
                return *std::any_cast<TargetType>(val);
            }
        }
    }
}

template<typename Func, typename Tuple, size_t... Is>
auto call_with_deps_impl(Func& func, Request& req, Response& res, std::index_sequence<Is...>) {
    auto deps = std::make_tuple(detail::resolve_arg<std::tuple_element_t<Is, Tuple>, Is, Tuple>(req, res)...);
    return func(detail::unwrap_arg<std::tuple_element_t<Is, Tuple>>(std::get<Is>(deps), req, res)...);
}

template<typename Func>
auto inject_and_call(Func& func, Request& req, Response& res) {
    using Traits = function_traits<Func>;
    using ArgsTuple = Traits::args_tuple;
    
    return call_with_deps_impl<Func, ArgsTuple>(
        func, 
        req, 
        res, 
        std::make_index_sequence<std::tuple_size_v<ArgsTuple>>{}
    );
}

} // namespace blaze
