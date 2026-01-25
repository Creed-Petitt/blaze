#ifndef BLAZE_REFLECTION_H
#define BLAZE_REFLECTION_H

#include <blaze/openapi.h>
#include <blaze/injector.h>
#include <blaze/wrappers.h>
#include <blaze/router.h>
#include <string>
#include <vector>

namespace blaze::reflection {

template<typename T> struct mp_type_wrapper { using type = T; };

// Helper to check if a type is a wrapper
template<typename T> struct is_wrapper : std::false_type {};
template<typename T> struct is_wrapper<Path<T>> : std::true_type {};
template<typename T> struct is_wrapper<Body<T>> : std::true_type {};
template<typename T> struct is_wrapper<Query<T>> : std::true_type {};

template<typename Func>
openapi::RouteDoc inspect_handler(const std::string& method, const std::string& path) {
    openapi::RouteDoc doc;
    doc.method = method;
    doc.path = path;
    doc.summary = "Auto-generated route documentation";

    using Traits = function_traits<Func>;
    using ArgsList = typename Traits::args_type;
    
    // 1. Inspect Return Type (Response)
    using Ret = typename Traits::return_type;
    // Extract T from Async<T>
    if constexpr (std::is_same_v<Ret, Async<void>>) {
        doc.response_schema = {{"type", "string"}, {"description", "Empty Response"}};
    } else {
        // Placeholder for more complex response schema generation
        doc.response_schema = {{"type", "object"}};
    }

    // 2. Inspect Arguments (Inputs)
    boost::mp11::mp_for_each<boost::mp11::mp_transform<mp_type_wrapper, ArgsList>>([&](auto arg_wrapper) {
        using T = typename decltype(arg_wrapper)::type;
        using PureT = std::remove_cvref_t<T>;

        if constexpr (is_instantiation_of<Body, PureT>::value) {
            using Inner = typename PureT::value_type;
            doc.request_body = openapi::generate_schema<Inner>();
        } else if constexpr (is_instantiation_of<Path, PureT>::value) {
            using Inner = typename PureT::value_type;
            doc.path_params.push_back({"param", openapi::generate_schema<Inner>()});
        } else if constexpr (is_instantiation_of<Query, PureT>::value) {
            using Inner = typename PureT::value_type;
            doc.query_params.push_back({"query", openapi::generate_schema<Inner>()});
        }
    });

    return doc;
}

} // namespace blaze::reflection

#endif
