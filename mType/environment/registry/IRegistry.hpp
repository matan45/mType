#pragma once
#include "../IEnvironmentComponent.hpp"
#include <string>
#include <vector>
#include <memory>

namespace environment::registry
{
    template <typename T>
    class IRegistry : public IEnvironmentComponent
    {
    public:
        virtual ~IRegistry() = default;
        
        virtual void registerItem(const std::string& name, std::shared_ptr<T> item) = 0;
        virtual std::shared_ptr<T> findItem(const std::string& name) const = 0;
        virtual bool hasItem(const std::string& name) const = 0;
        virtual void removeItem(const std::string& name) = 0;
        virtual std::vector<std::string> getAllItemNames() const = 0;
        virtual size_t getItemCount() const = 0;
        
        void initialize() override {}
        void cleanup() override {}
    };
}