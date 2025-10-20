#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include "../circularDependency/CircularDependencyDetector.hpp"

namespace parser
{
    /// @brief Manages type declarations, inheritance tracking, and validation
    /// Responsible for tracking classes, interfaces, functions and their relationships
    class TypeRegistry
    {
    private:
        // Track declared class/interface names to prevent duplicates
        std::unordered_set<std::string> declaredTypeNames;

        // Track classes and interfaces separately for validation
        std::unordered_set<std::string> declaredClasses;
        std::unordered_set<std::string> declaredInterfaces;

        // Track final classes and interfaces
        std::unordered_set<std::string> finalClasses;
        std::unordered_set<std::string> finalInterfaces;

        // Track inheritance relationships for circular detection
        std::unordered_map<std::string, std::string> classParents; // childClass -> parentClass
        std::unordered_map<std::string, std::vector<std::string>> interfaceParents;
        // childInterface -> [parentInterfaces]

        // Track declared global function names to prevent duplicates
        std::unordered_set<std::string> declaredFunctionNames;

        // Circular dependency detection for inheritance validation
        std::unique_ptr<circularDependency::CircularDependencyDetector> dependencyDetector;

    public:
        TypeRegistry();
        ~TypeRegistry() = default;

        // Type name tracking for duplicate detection
        [[nodiscard]] bool isTypeDeclared(const std::string& typeName) const noexcept;
        void registerTypeName(const std::string& typeName) noexcept;
        void clearDeclaredTypes() noexcept;

        // Separate class/interface tracking for validation
        [[nodiscard]] bool isClassDeclared(const std::string& className) const noexcept;
        [[nodiscard]] bool isInterfaceDeclared(const std::string& interfaceName) const noexcept;
        void registerClass(const std::string& className, bool isFinal = false) noexcept;
        void registerInterface(const std::string& interfaceName, bool isFinal = false) noexcept;

        // Final modifier tracking
        [[nodiscard]] bool isClassFinal(const std::string& className) const noexcept;
        [[nodiscard]] bool isInterfaceFinal(const std::string& interfaceName) const noexcept;

        // Circular inheritance detection
        [[nodiscard]] bool registerClassInheritance(const std::string& childClass,
                                                    const std::string& parentClass) noexcept;
        [[nodiscard]] bool registerInterfaceInheritance(const std::string& childInterface,
                                                        const std::vector<std::string>& parentInterfaces) noexcept;
        [[nodiscard]] std::vector<std::string> getClassInheritanceChain(const std::string& className) const noexcept;

        // Global function name tracking for duplicate detection
        [[nodiscard]] bool isFunctionDeclared(const std::string& functionName) const noexcept;
        void registerFunctionName(const std::string& functionName) noexcept;
        void clearDeclaredFunctions() noexcept;

    private:
        // Helper methods for inheritance validation
        [[nodiscard]] std::string extractBaseName(const std::string& typeName) const noexcept;
        [[nodiscard]] std::vector<std::string> buildClassInheritanceChain(
            const std::string& childClass,
            const std::string& parentClass) const;
        [[nodiscard]] std::vector<std::string> buildInterfaceInheritanceChain(
            const std::string& childInterface,
            const std::vector<std::string>& parentInterfaces) const;
    };
}
