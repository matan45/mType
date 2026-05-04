#pragma once

#include <string>
#include <cstddef>
#include <vector>
#include <memory>
#include <functional>

// Forward declarations
namespace ast {
    namespace nodes {
        namespace classes {
            class MethodNode;
        }
    }
    class GenericType;
}

namespace runtimeTypes {
    namespace klass {
        class MethodDefinition;
    }
}

namespace vm {

    /**
     * @brief Represents a method signature for overload resolution
     *
     * Following Java/C# model:
     * - Signature = method name + parameter types (WITHOUT 'this')
     * - Used for compile-time overload resolution and runtime method lookup
     * - Eliminates confusion about implicit 'this' parameter
     *
     * Design principles:
     * - Instance method signatures NEVER include 'this' parameter
     * - Signatures are immutable once created
     * - Can be used as map keys (has equality and hash)
     */
    class MethodSignature {
    private:
        std::string methodName;
        std::vector<std::string> parameterTypes;  // WITHOUT 'this' for instance methods

    public:
        /**
         * @brief Constructor
         * @param name Method name
         * @param paramTypes Parameter types (WITHOUT 'this')
         */
        MethodSignature(const std::string& name, const std::vector<std::string>& paramTypes)
            : methodName(name), parameterTypes(paramTypes) {}

        /**
         * @brief Create signature from MethodDefinition
         * @param method The method definition (can be instance or static)
         * @return Method signature (without 'this' for instance methods)
         */
        static MethodSignature fromMethodDefinition(
            const runtimeTypes::klass::MethodDefinition* method);

        /**
         * @brief Create signature from AST MethodNode
         * @param node The method AST node
         * @param isStatic Whether the method is static
         * @return Method signature (without 'this' for instance methods)
         */
        static MethodSignature fromMethodNode(
            const ast::nodes::classes::MethodNode* node,
            bool isStatic);

        /**
         * @brief Create signature from method name and parameter count
         * @param name Method name
         * @param paramCount Parameter count (WITHOUT 'this')
         * @return Method signature
         */
        static MethodSignature fromNameAndCount(
            const std::string& name,
            size_t paramCount);

        /**
         * @brief Generate mangled name for bytecode storage/lookup
         * @param className The class name
         * @param isStatic Whether the method is static
         * @return Mangled name like "ClassName::methodName" or "ClassName::methodName/Type1,Type2"
         */
        std::string toMangledName(const std::string& className, bool isStatic) const;

        /**
         * @brief Get method name
         */
        const std::string& getMethodName() const { return methodName; }

        /**
         * @brief Get parameter types
         */
        const std::vector<std::string>& getParameterTypes() const { return parameterTypes; }

        /**
         * @brief Get parameter count (always WITHOUT 'this')
         */
        size_t getParameterCount() const { return parameterTypes.size(); }

        /**
         * @brief Check if signature has no parameters
         */
        bool hasNoParameters() const { return parameterTypes.empty(); }

        /**
         * @brief Equality comparison for map storage
         */
        bool operator==(const MethodSignature& other) const {
            return methodName == other.methodName &&
                   parameterTypes == other.parameterTypes;
        }

        /**
         * @brief Inequality comparison
         */
        bool operator!=(const MethodSignature& other) const {
            return !(*this == other);
        }

        /**
         * @brief Hash function for unordered_map storage
         */
        size_t hash() const;

        /**
         * @brief Convert to string for debugging/error messages
         * @return String like "methodName(int, String)" or "methodName()"
         */
        std::string toString() const;
    };

} // namespace vm

// Hash specialization for std::unordered_map
namespace std {
    template<>
    struct hash<vm::MethodSignature> {
        size_t operator()(const vm::MethodSignature& sig) const {
            return sig.hash();
        }
    };
}
