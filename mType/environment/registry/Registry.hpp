#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>

namespace environment::registry
{
    /**
     * @brief Template base class for registry pattern implementation
     *
     * Provides common registry functionality for managing items of type T.
     * This eliminates code duplication between ClassRegistry, FunctionRegistry, etc.
     *
     * @tparam T The type of items to store in the registry (e.g., ClassDefinition, FunctionDefinition)
     */
    template<typename T>
    class Registry
    {
    protected:
        std::unordered_map<std::string, std::shared_ptr<T>> items;

    public:
        Registry() = default;
        virtual ~Registry() = default;

        /**
         * @brief Register an item with the given name
         * @param name The unique name for the item
         * @param item The item to register
         */
        void registerItem(const std::string& name, std::shared_ptr<T> item)
        {
            items[name] = item;
        }

        /**
         * @brief Find an item by name
         * @param name The name of the item to find
         * @return Shared pointer to the item, or nullptr if not found
         */
        std::shared_ptr<T> findItem(const std::string& name) const
        {
            auto it = items.find(name);
            return (it != items.end()) ? it->second : nullptr;
        }

        /**
         * @brief Check if an item exists in the registry
         * @param name The name of the item to check
         * @return true if the item exists, false otherwise
         */
        bool hasItem(const std::string& name) const
        {
            return items.find(name) != items.end();
        }

        /**
         * @brief Remove an item from the registry
         * @param name The name of the item to remove
         */
        virtual void removeItem(const std::string& name)
        {
            items.erase(name);
        }

        /**
         * @brief Get all item names in sorted order
         * @return Vector of all item names, sorted alphabetically
         */
        std::vector<std::string> getAllItemNames() const
        {
            std::vector<std::string> names;
            names.reserve(items.size());

            for (const auto& [name, _] : items)
            {
                names.push_back(name);
            }

            std::sort(names.begin(), names.end());
            return names;
        }

        /**
         * @brief Get the total number of items in the registry
         * @return The number of registered items
         */
        size_t getItemCount() const
        {
            return items.size();
        }

        /**
         * @brief Get the component name for this registry
         * Must be implemented by derived classes
         * @return The component name as a string
         */
        virtual std::string getComponentName() const = 0;
    };
}
