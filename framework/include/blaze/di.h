#ifndef BLAZE_DI_H
#define BLAZE_DI_H

#include <memory>
#include <unordered_map>
#include <functional>
#include <typeindex>
#include <stdexcept>
#include <string>
#include <any>
#include <iostream>
#include <tuple>
#include <type_traits>
#include <mutex>

namespace blaze {

#define BLAZE_DEPS(...) using BlazeDeps = std::tuple<__VA_ARGS__>;

class ServiceProvider;

struct ServiceDescriptor {
    using Factory = std::function<std::any(ServiceProvider&)>;
    
    Factory factory;
    bool is_singleton;
    std::any instance;
    std::shared_ptr<std::mutex> init_mutex = std::make_shared<std::mutex>(); 
};

template <typename T>
std::shared_ptr<T> auto_wire_factory(ServiceProvider& sp);


class ServiceProvider {
    std::unordered_map<std::type_index, ServiceDescriptor> services_;

public:
    ServiceProvider() = default;

    template<typename T>
    void provide(std::function<std::shared_ptr<T>(ServiceProvider&)> factory) {
        ServiceDescriptor desc;
        desc.is_singleton = true;
        desc.factory = [factory](ServiceProvider& sp) { return factory(sp); };
        services_[std::type_index(typeid(T))] = std::move(desc);
    }

    template<typename T>
    void provide(std::shared_ptr<T> instance) {
        ServiceDescriptor desc;
        desc.is_singleton = true;
        desc.instance = instance;
        desc.factory = [instance](ServiceProvider&) { return instance; };
        services_[std::type_index(typeid(T))] = std::move(desc);
    }

    template<typename T>
    void provide() {
        provide<T>([](ServiceProvider& sp) {
            return auto_wire_factory<T>(sp);
        });
    }
    
    template<typename T>
    void provide_transient(std::function<std::shared_ptr<T>(ServiceProvider&)> factory) {
        ServiceDescriptor desc;
        desc.is_singleton = false;
        desc.factory = [factory](ServiceProvider& sp) { return factory(sp); };
        services_[std::type_index(typeid(T))] = std::move(desc);
    }

    template<typename T>
    void provide_transient() {
        provide_transient<T>([](ServiceProvider& sp) {
            return auto_wire_factory<T>(sp);
        });
    }


    template<typename T>
    std::shared_ptr<T> resolve() {
        auto type_idx = std::type_index(typeid(T));
        auto it = services_.find(type_idx);

        if (it == services_.end()) {
            throw std::runtime_error(std::string("Service not registered: ") + typeid(T).name());
        }

        auto& descriptor = it->second;

        if (descriptor.is_singleton) {
            if (descriptor.instance.has_value()) {
                return std::any_cast<std::shared_ptr<T>>(descriptor.instance);
            }

            std::lock_guard<std::mutex> lock(*descriptor.init_mutex);
            
            if (!descriptor.instance.has_value()) {
                descriptor.instance = descriptor.factory(*this);
            }
            return std::any_cast<std::shared_ptr<T>>(descriptor.instance);
        }

        return std::any_cast<std::shared_ptr<T>>(descriptor.factory(*this));
    }

    template<typename T>
    bool has() const {
        return services_.find(std::type_index(typeid(T))) != services_.end();
    }
};

template <typename T, typename = void>
struct has_blaze_deps : std::false_type {};

template <typename T>
struct has_blaze_deps<T, std::void_t<typename T::BlazeDeps>> : std::true_type {};

template <typename T>
inline constexpr bool has_blaze_deps_v = has_blaze_deps<T>::value;

template <typename T, typename Tuple, size_t... Is>
std::shared_ptr<T> auto_wire_impl(ServiceProvider& sp, std::index_sequence<Is...>) {
    return std::make_shared<T>(
        sp.resolve<typename std::tuple_element<Is, Tuple>::type>()...
    );
}

template <typename T>
std::shared_ptr<T> auto_wire_factory(ServiceProvider& sp) {
    if constexpr (has_blaze_deps_v<T>) {
        using DepsTuple = typename T::BlazeDeps;
        return auto_wire_impl<T, DepsTuple>(
            sp, 
            std::make_index_sequence<std::tuple_size_v<DepsTuple>>{}
        );
    } else {
        return std::make_shared<T>();
    }
}

} // namespace blaze

#endif
