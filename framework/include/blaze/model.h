#ifndef BLAZE_MODEL_H
#define BLAZE_MODEL_H

#include <boost/describe.hpp>
#include <boost/mp11.hpp>
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

#endif
