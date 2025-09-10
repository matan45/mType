#pragma once

#include "CollectionMethodValidator.hpp"
#include "../../runtimeTypes/collections/Array.hpp"
#include "../../runtimeTypes/collections/Map.hpp"
#include "../../runtimeTypes/collections/Set.hpp"
#include "../../runtimeTypes/collections/Stack.hpp"
#include "../../runtimeTypes/collections/Queue.hpp"
#include "../../errors/TypeException.hpp"
#include <memory>
#include <string>
#include <vector>
#include <type_traits>
#include <optional>

namespace evaluator::utils
{
    using namespace value;
    using namespace errors;
    using namespace runtimeTypes::collections;

    /**
     * @brief Unified template-based collection method dispatcher
     * Replaces 5 separate collection method handlers with a single template implementation
     * This eliminates ~350 lines of duplicate code
     */
    template<typename CollectionType>
    class CollectionMethodDispatcher
    {
    public:
        /**
         * @brief Main dispatch method for all collection operations
         * @param collection The collection to operate on
         * @param methodName The method to call
         * @param args Arguments for the method
         * @return Result of the method call
         */
        static Value dispatch(std::shared_ptr<CollectionType> collection, 
                            const std::string& methodName, 
                            const std::vector<Value>& args)
        {
            // Handle common methods first (available on all collections)
            if (auto result = handleCommonMethods(collection, methodName, args)) {
                return *result;
            }

            // Dispatch to type-specific handlers using if constexpr
            if constexpr (std::is_same_v<CollectionType, Array>) {
                return handleArrayMethods(collection, methodName, args);
            }
            else if constexpr (std::is_same_v<CollectionType, Map>) {
                return handleMapMethods(collection, methodName, args);
            }
            else if constexpr (std::is_same_v<CollectionType, Set>) {
                return handleSetMethods(collection, methodName, args);
            }
            else if constexpr (std::is_same_v<CollectionType, Stack>) {
                return handleStackMethods(collection, methodName, args);
            }
            else if constexpr (std::is_same_v<CollectionType, Queue>) {
                return handleQueueMethods(collection, methodName, args);
            }
            else {
                throw TypeException("Unknown collection type for method dispatch");
            }
        }

    private:
        /**
         * @brief Handle methods common to ALL collections (size, empty, clear)
         * @return std::optional with result if method was handled, std::nullopt otherwise
         */
        static std::optional<Value> handleCommonMethods(std::shared_ptr<CollectionType> collection,
                                                       const std::string& methodName,
                                                       const std::vector<Value>& args)
        {
            if (methodName == "size") {
                CollectionMethodValidator::validateNoArgs(methodName, args);
                return static_cast<int>(collection->size());
            }
            else if (methodName == "empty") {
                CollectionMethodValidator::validateNoArgs(methodName, args);
                return collection->empty();
            }
            else if (methodName == "clear") {
                CollectionMethodValidator::validateNoArgs(methodName, args);
                collection->clear();
                return std::monostate{};
            }
            
            return std::nullopt; // Method not handled
        }

        /**
         * @brief Handle Array-specific methods
         */
        static Value handleArrayMethods(std::shared_ptr<Array> array,
                                      const std::string& methodName,
                                      const std::vector<Value>& args)
        {
            if (methodName == "get") {
                CollectionMethodValidator::validateArgCount(methodName, args, 1);
                size_t index = static_cast<size_t>(CollectionMethodValidator::extractIntIndex(args[0], "Array index"));
                return array->get(index);
            }
            else if (methodName == "set") {
                CollectionMethodValidator::validateArgCount(methodName, args, 2);
                size_t index = static_cast<size_t>(CollectionMethodValidator::extractIntIndex(args[0], "Array index"));
                array->set(index, args[1]);
                return std::monostate{};
            }
            else if (methodName == "add" || methodName == "push") {
                CollectionMethodValidator::validateArgCount(methodName, args, 1);
                array->add(args[0]);
                return std::monostate{};
            }
            else if (methodName == "removeAt") {
                CollectionMethodValidator::validateArgCount(methodName, args, 1);
                size_t index = static_cast<size_t>(CollectionMethodValidator::extractIntIndex(args[0], "Array index"));
                array->removeAt(index);
                return std::monostate{};
            }
            // Note: Array doesn't have a contains method
            else {
                throw TypeException("Unknown method '" + methodName + "' for Array type");
            }
        }

        /**
         * @brief Handle Map-specific methods
         */
        static Value handleMapMethods(std::shared_ptr<Map> map,
                                    const std::string& methodName,
                                    const std::vector<Value>& args)
        {
            if (methodName == "get") {
                CollectionMethodValidator::validateArgCount(methodName, args, 1);
                std::string key = CollectionMethodValidator::extractStringKey(args[0]);
                return map->get(key);
            }
            else if (methodName == "put") {
                CollectionMethodValidator::validateArgCount(methodName, args, 2);
                std::string key = CollectionMethodValidator::extractStringKey(args[0]);
                map->put(key, args[1]);
                return std::monostate{};
            }
            else if (methodName == "containsKey") {
                CollectionMethodValidator::validateArgCount(methodName, args, 1);
                std::string key = CollectionMethodValidator::extractStringKey(args[0]);
                return map->containsKey(key);
            }
            else if (methodName == "remove") {
                CollectionMethodValidator::validateArgCount(methodName, args, 1);
                std::string key = CollectionMethodValidator::extractStringKey(args[0]);
                map->remove(key);
                return std::monostate{};
            }
            else if (methodName == "keySet") {
                CollectionMethodValidator::validateNoArgs(methodName, args);
                return map->keySet();
            }
            else {
                throw TypeException("Unknown method '" + methodName + "' for Map type");
            }
        }

        /**
         * @brief Handle Set-specific methods
         */
        static Value handleSetMethods(std::shared_ptr<Set> set,
                                    const std::string& methodName,
                                    const std::vector<Value>& args)
        {
            if (methodName == "add") {
                CollectionMethodValidator::validateArgCount(methodName, args, 1);
                return set->add(args[0]);
            }
            else if (methodName == "contains") {
                CollectionMethodValidator::validateArgCount(methodName, args, 1);
                return set->contains(args[0]);
            }
            else if (methodName == "remove") {
                CollectionMethodValidator::validateArgCount(methodName, args, 1);
                return set->remove(args[0]);
            }
            else {
                throw TypeException("Unknown method '" + methodName + "' for Set type");
            }
        }

        /**
         * @brief Handle Stack-specific methods
         */
        static Value handleStackMethods(std::shared_ptr<Stack> stack,
                                      const std::string& methodName,
                                      const std::vector<Value>& args)
        {
            if (methodName == "push") {
                CollectionMethodValidator::validateArgCount(methodName, args, 1);
                stack->push(args[0]);
                return std::monostate{};
            }
            else if (methodName == "pop") {
                CollectionMethodValidator::validateNoArgs(methodName, args);
                return stack->pop();
            }
            else if (methodName == "top") {
                CollectionMethodValidator::validateNoArgs(methodName, args);
                return stack->top();
            }
            else {
                throw TypeException("Unknown method '" + methodName + "' for Stack type");
            }
        }

        /**
         * @brief Handle Queue-specific methods
         */
        static Value handleQueueMethods(std::shared_ptr<Queue> queue,
                                      const std::string& methodName,
                                      const std::vector<Value>& args)
        {
            if (methodName == "enqueue") {
                CollectionMethodValidator::validateArgCount(methodName, args, 1);
                queue->enqueue(args[0]);
                return std::monostate{};
            }
            else if (methodName == "dequeue") {
                CollectionMethodValidator::validateNoArgs(methodName, args);
                return queue->dequeue();
            }
            else if (methodName == "front") {
                CollectionMethodValidator::validateNoArgs(methodName, args);
                return queue->front();
            }
            else {
                throw TypeException("Unknown method '" + methodName + "' for Queue type");
            }
        }
    };
}