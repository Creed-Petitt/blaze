#pragma once

#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace blaze {

class Services {
    std::unordered_map<std::type_index, std::shared_ptr<void>> services_;
    mutable std::mutex mutex_;

public:
    Services() = default;
    Services(const Services&) = delete;
    Services& operator=(const Services&) = delete;

    template<typename T>
    void add(std::shared_ptr<T> instance) {
        if (!instance) {
            throw std::invalid_argument("Cannot register null service");
        }

        std::lock_guard lock(mutex_);
        services_[std::type_index(typeid(T))] = std::move(instance);
    }

    template<typename T, typename... Args>
    std::shared_ptr<T> emplace(Args&&... args) {
        auto instance = std::make_shared<T>(std::forward<Args>(args)...);
        add<T>(instance);
        return instance;
    }

    template<typename T>
    std::shared_ptr<T> get() const {
        std::lock_guard lock(mutex_);
        const auto it = services_.find(std::type_index(typeid(T)));
        if (it == services_.end()) {
            throw std::runtime_error(std::string("Service not registered: ") + typeid(T).name());
        }
        return std::static_pointer_cast<T>(it->second);
    }

    template<typename T>
    bool has() const {
        std::lock_guard lock(mutex_);
        return services_.contains(std::type_index(typeid(T)));
    }
};

using ServiceProvider = Services;

} // namespace blaze