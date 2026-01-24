#ifndef BLAZE_MODEL_H
#define BLAZE_MODEL_H

#include <boost/describe.hpp>
#include <boost/mp11.hpp>
#include <boost/json.hpp>
#include <string>
#include <string_view>
#include <type_traits>

// usage: BLAZE_MODEL(User, id, name)
#define BLAZE_MODEL(Type, ...) BOOST_DESCRIBE_STRUCT(Type, (), (__VA_ARGS__))

namespace blaze {

    template<typename T, typename RowType>
    T row_to_struct(const RowType& row) {
        T obj;

        using members = boost::describe::describe_members<T, boost::describe::mod_any_access>;

        boost::mp11::mp_for_each<members>([&](auto D) {
            auto cell = row[D.name];
            using MemberType = typename std::remove_reference<decltype(obj.*(D.pointer))>::type;
            obj.*(D.pointer) = cell.template as<MemberType>();
        });

        return obj;
    }

} // namespace blaze

// Global tag_invoke for Boost.JSON -> Boost.Describe serialization
namespace boost::json {

template<class T>
typename std::enable_if<
    boost::describe::has_describe_members<T>::value,
    void
>::type
tag_invoke(value_from_tag, value& jv, T const& t) {
    object obj;
    boost::mp11::mp_for_each<boost::describe::describe_members<T, boost::describe::mod_any_access>>(
        [&](auto D) {
            obj[D.name] = value_from(t.*D.pointer);
        }
    );
    jv = std::move(obj);
}

template<class T>
typename std::enable_if<
    boost::describe::has_describe_members<T>::value,
    T
>::type
tag_invoke(value_to_tag<T>, value const& jv) {
    object const& obj = jv.as_object();
    T t{};
    boost::mp11::mp_for_each<boost::describe::describe_members<T, boost::describe::mod_any_access>>(
        [&](auto D) {
            if(obj.contains(D.name)) {
                t.*D.pointer = value_to<typename std::remove_reference<decltype(t.*D.pointer)>::type>(obj.at(D.name));
            }
        }
    );
    return t;
}

} // namespace boost::json

#endif
