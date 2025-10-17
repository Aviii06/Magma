#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <typeindex>
#include <type_traits>

#include <logging/Logging.h>

namespace Magma
{
    class IService
    {
    public:
        virtual ~IService() = default;

        virtual void Cleanup() {}
    };

    class ServiceLocator
    {
    public:
        template<typename T>
        static void Register(std::shared_ptr<T> service)
        {
            static_assert(std::is_base_of<IService, T>::value, "T must inherit from IService");
            std::lock_guard lock(s_mutex);
            auto typeIndex = std::type_index(typeid(T));

            if (s_services.find(typeIndex) != s_services.end())
            {
                Logger::Log(LogLevel::WARNING, "Service already registered, skipping");
                return;
            }

            Logger::Log(LogLevel::INFO, "Registering service");
            s_services[typeIndex] = service;
        }

        template<typename T>
        static void Unregister()
        {
            std::lock_guard lock(s_mutex);
            auto typeIndex = std::type_index(typeid(T));

            auto it = s_services.find(typeIndex);
            if (it == s_services.end())
            {
                Logger::Log(LogLevel::WARNING, "Service not registered");
                return;
            }

            Logger::Log(LogLevel::INFO, "Unregistering service");
            it->second->Cleanup();
            s_services.erase(it);
        }

        template<typename T>
        static std::shared_ptr<T> Get()
        {
            std::lock_guard lock(s_mutex);
            auto typeIndex = std::type_index(typeid(T));

            auto it = s_services.find(typeIndex);
            if (it == s_services.end())
            {
                throw std::runtime_error("Service not found");
            }

            return std::static_pointer_cast<T>(it->second);
        }

        static void ShutdownServices()
        {
            std::lock_guard lock(s_mutex);
            Logger::Log(LogLevel::INFO, "Shutting down all services");

            for (auto& [typeIndex, service] : s_services)
            {
                service->Cleanup();
            }

            s_services.clear();
        }

    private:
        static inline std::map<std::type_index, std::shared_ptr<IService>> s_services;
        static inline std::mutex s_mutex;

    };
}
