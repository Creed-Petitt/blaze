#ifndef BLAZE_TRAITS_H
#define BLAZE_TRAITS_H

#include <type_traits>
#include <tuple>
#include <memory>
#include <boost/mp11.hpp>

namespace blaze {

    // Helper to check if a type is a template instance of a specific template
    template <template <typename...> class Template, typename T>
    struct is_instantiation_of : std::false_type {};

    template <template <typename...> class Template, typename... Args>
    struct is_instantiation_of<Template, Template<Args...>> : std::true_type {};

    // Function Traits to extract args and return type
    template <typename T>
    struct function_traits : public function_traits<decltype(&T::operator())> {};

    template <typename ClassType, typename ReturnType, typename... Args>
    struct function_traits<ReturnType(ClassType::*)(Args...) const> {
        using args_tuple = std::tuple<Args...>;
        using args_type = boost::mp11::mp_list<Args...>;
        using return_type = ReturnType;
    };

    template <typename ReturnType, typename... Args>
    struct function_traits<ReturnType(*)(Args...)> {
        using args_tuple = std::tuple<Args...>;
        using args_type = boost::mp11::mp_list<Args...>;
        using return_type = ReturnType;
    };

    template<typename T>
    struct is_shared_ptr : std::false_type {};

    template<typename T>
    struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};

    template<typename T>
    inline constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

    // Helper to count how many times a template appears in a tuple before index N
    template <template <typename...> class Template, size_t N, typename Tuple>
    struct count_instances_before {
        static constexpr size_t count() {
            return count_impl(std::make_index_sequence<N>{});
        }
    private:
        template <size_t... Is>
        static constexpr size_t count_impl(std::index_sequence<Is...>) {
            return (0 + ... + (is_instantiation_of<Template, std::remove_cvref_t<std::tuple_element_t<Is, Tuple>>>::value ? 1 : 0));
        }
    };

} // namespace blaze

#endif
