#ifndef BLAZE_OPENAPI_H
#define BLAZE_OPENAPI_H

#include <boost/json.hpp>
#include <boost/describe.hpp>
#include <string>
#include <vector>
#include <type_traits>
#include <blaze/wrappers.h>

namespace blaze::openapi {

using namespace boost::json;

template<typename T>
std::string get_type_name() {
    if constexpr (std::is_integral_v<T>) return "integer";
    else if constexpr (std::is_floating_point_v<T>) return "number";
    else if constexpr (std::is_same_v<T, std::string>) return "string";
    else if constexpr (std::is_same_v<T, bool>) return "boolean";
    else return "object";
}

// --- SCHEMA GENERATOR ---
template<typename T, typename = void>
struct schema_builder {
    static object build() {
        using PureT = std::remove_cvref_t<T>;
        object schema;

        if constexpr (std::is_same_v<PureT, std::string>) {
            schema["type"] = "string";
        } else if constexpr (std::is_integral_v<PureT>) {
            schema["type"] = "integer";
        } else if constexpr (std::is_floating_point_v<PureT>) {
            schema["type"] = "number";
        } else if constexpr (std::is_same_v<PureT, bool>) {
            schema["type"] = "boolean";
        } else if constexpr (boost::mp11::mp_is_list<PureT>::value) {
             // Handle vectors/lists
             schema["type"] = "array";
             // Todo: inner type schema
        } else {
            // Default: Reflect on Struct
            schema["type"] = "object";
            object props;
            if constexpr (boost::describe::has_describe_members<PureT>::value) {
                using Members = boost::describe::describe_members<PureT, boost::describe::mod_any_access>;
                boost::mp11::mp_for_each<Members>([&](auto meta) {
                    using FieldT = std::remove_cvref_t<decltype(std::declval<PureT>().*meta.pointer)>;
                    props[meta.name] = schema_builder<FieldT>::build();
                });
            }
            schema["properties"] = props;
        }
        return schema;
    }
};

// Specialized for Wrappers (Path, Body, etc.) to unwrap them
template<template<typename> typename W, typename T>
struct schema_builder<W<T>, std::void_t<typename W<T>::value_type>> {
    static object build() {
        return schema_builder<T>::build();
    }
};

// Specialized for std::vector
template<typename T>
struct schema_builder<std::vector<T>> {
    static object build() {
        return {
            {"type", "array"},
            {"items", schema_builder<T>::build()}
        };
    }
};


template<typename T>
object generate_schema() {
    return schema_builder<std::remove_cvref_t<T>>::build();
}

struct RouteDoc {
    std::string method;
    std::string path;
    std::string summary;
    object request_body;
    std::vector<std::pair<std::string, object>> path_params;
    std::vector<std::pair<std::string, object>> query_params;
    object response_schema;
};

} // namespace blaze::openapi

#endif
