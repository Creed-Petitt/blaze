#ifndef BLAZE_INJECTOR_H
#define BLAZE_INJECTOR_H

#include <blaze/di.h>
#include <blaze/request.h>
#include <blaze/response.h>
#include <blaze/model.h>
#include <blaze/wrappers.h>
#include <blaze/repository.h>
#include <blaze/traits.h>
#include <tuple>
#include <functional>
#include <type_traits>
#include <memory>
#include <charconv>
#include <boost/mp11.hpp>

namespace blaze {

// String to Type conversion helper
template<typename T>
T convert_string(const std::string& s) {
    if constexpr (std::is_same_v<T, std::string>) {
        return s;
    } else if constexpr (std::is_same_v<T, bool>) {
        return (s == "true" || s == "1" || s == "yes");
    } else if constexpr (std::is_integral_v<T>) {
        T val;
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
        if (ec != std::errc()) return T{};
        return val;
    } else if constexpr (std::is_floating_point_v<T>) {
        return static_cast<T>(std::stod(s));
    } else {
        return T{};
    }
}

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

    // 1. Resolve a single argument into a std::any (shared_ptr<T>)
    template<typename ArgType, size_t Is, typename Tuple>
    std::any resolve_arg(ServiceProvider& provider, Request& req, Response& res) {
        using PureType = std::remove_cvref_t<ArgType>;
        
        if constexpr (std::is_same_v<PureType, Request> || std::is_same_v<PureType, Response>) {
            return nullptr; // Placeholders
        } else if constexpr (is_instantiation_of<Path, PureType>::value) {
            using InnerT = typename PureType::value_type;
            static constexpr size_t idx = count_instances_before<Path, Is, Tuple>::count();
            if (idx < req.path_values.size()) {
                return std::make_shared<PureType>(convert_string<InnerT>(req.path_values[idx]));
            }
            return std::make_shared<PureType>();
        } else if constexpr (is_instantiation_of<Body, PureType>::value) {
            using InnerT = typename PureType::value_type;
            auto model = req.json<InnerT>();
            try_validate(model);
            return std::make_shared<PureType>(std::move(model));
        } else if constexpr (is_instantiation_of<Query, PureType>::value) {
            using InnerT = PureType::value_type;
            InnerT model{};
            using Members = boost::describe::describe_members<InnerT, boost::describe::mod_any_access>;
            boost::mp11::mp_for_each<Members>([&](auto meta) {
                std::string key = meta.name;
                if (req.query.count(key)) {
                    using FieldT = std::remove_cvref_t<decltype(model.*meta.pointer)>;
                    model.*meta.pointer = convert_string<FieldT>(req.query.at(key));
                }
            });
            try_validate(model);
            return std::make_shared<PureType>(model);
        } else if constexpr (is_instantiation_of<Context, PureType>::value) {
            using InnerT = typename PureType::value_type;
            auto val = req.get_opt<InnerT>(typeid(InnerT).name());
            if (val) return std::make_shared<PureType>(*val);
            return std::make_shared<PureType>();
        } else if constexpr (is_instantiation_of<Repository, PureType>::value) {
            auto db = provider.resolve<Database>();
            return std::make_shared<PureType>(db);
        } else if constexpr (is_shared_ptr_v<PureType>) {
            return provider.resolve<typename PureType::element_type>();
        } else {
            if (provider.has<PureType>()) {
                return provider.resolve<PureType>();
            }
            if constexpr (boost::describe::has_describe_members<PureType>::value) {
                auto model = req.json<PureType>();
                try_validate(model);
                return std::make_shared<PureType>(std::move(model));
            }
            return provider.resolve<PureType>();
        }
    }

    // 2. Unwrap the std::any into the exact type the function needs
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
auto call_with_deps_impl(Func& func, ServiceProvider& provider, Request& req, Response& res, std::index_sequence<Is...>) {
    auto deps = std::make_tuple(detail::resolve_arg<std::tuple_element_t<Is, Tuple>, Is, Tuple>(provider, req, res)...);
    return func(detail::unwrap_arg<std::tuple_element_t<Is, Tuple>>(std::get<Is>(deps), req, res)...);
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
