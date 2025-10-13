#pragma once
#include "../ast/ASTNode.hpp"
#include <memory>
#include <optional>
#include <functional>
#include <unordered_set>
#include <string>

namespace parser
{
    class StatementParser;
    class ExpressionParser;
    class ClassParser;
    class InterfaceParser;
    class TokenStream;

    using namespace ast;

    /// @brief Enables inter-parser communication without circular dependencies
    /// Provides clean interface for parser delegation with memory-safe references
    class ParseContext
    {
    private:
        std::optional<std::reference_wrapper<StatementParser>> statementParser;
        std::optional<std::reference_wrapper<ExpressionParser>> expressionParser;
        std::optional<std::reference_wrapper<ClassParser>> classParser;
        std::optional<std::reference_wrapper<InterfaceParser>> interfaceParser;
        std::optional<std::reference_wrapper<TokenStream>> tokenStream;

        // Context flags
        bool insideLambdaBody = false;
        bool insideAsyncFunction = false;  // NEW: Track if we're inside an async function/method
        bool insideFunctionBody = false;   // Track if we're inside a function body
        bool insideClassBody = false;      // Track if we're inside a class body
        bool insideInterfaceBody = false;  // Track if we're inside an interface body
        bool insideConstructorBody = false; // Track if we're inside a constructor body

        // NEW: Track declared class/interface names to prevent duplicates
        std::unordered_set<std::string> declaredTypeNames;

        // NEW: Track classes and interfaces separately for validation
        std::unordered_set<std::string> declaredClasses;
        std::unordered_set<std::string> declaredInterfaces;

        // NEW: Track final classes and interfaces
        std::unordered_set<std::string> finalClasses;
        std::unordered_set<std::string> finalInterfaces;

        // NEW: Track inheritance relationships for circular detection
        std::unordered_map<std::string, std::string> classParents;  // childClass -> parentClass
        std::unordered_map<std::string, std::vector<std::string>> interfaceParents;  // childInterface -> [parentInterfaces]

        // NEW: Track declared global function names to prevent duplicates
        std::unordered_set<std::string> declaredFunctionNames;

    public:
        /// @brief Default constructor for delayed initialization
        explicit ParseContext() = default;

        /// @brief Constructor with immediate initialization
        explicit ParseContext(StatementParser& stmt, ExpressionParser& expr, ClassParser& cls, InterfaceParser& iface,
                     TokenStream& stream);

        ~ParseContext() = default;

        /// @brief Parse a statement using StatementParser
        [[nodiscard]] std::unique_ptr<ASTNode> parseStatement();

        /// @brief Parse an expression using ExpressionParser
        [[nodiscard]] std::unique_ptr<ASTNode> parseExpression();

        /// @brief Parse a class using ClassParser
        [[nodiscard]] std::unique_ptr<ASTNode> parseClass();

        /// @brief Parse an interface using InterfaceParser
        [[nodiscard]] std::unique_ptr<ASTNode> parseInterface();

        /// @brief Parse a new expression using ClassParser
        [[nodiscard]] std::unique_ptr<ASTNode> parseNewExpression();

        // Setters for delayed initialization with memory-safe references
        void setStatementParser(StatementParser& parser);
        void setExpressionParser(ExpressionParser& parser);
        void setClassParser(ClassParser& parser);
        void setInterfaceParser(InterfaceParser& parser);
        void setTokenStream(TokenStream& stream);

        // NEW: Async context management
        /// @brief Check if we're currently inside an async function/method
        [[nodiscard]] bool isInsideAsyncFunction() const { return insideAsyncFunction; }

        /// @brief Set async function context (for entering async function body)
        void setAsyncContext(bool async) { insideAsyncFunction = async; }

        /// @brief RAII helper for async context management
        class AsyncContextGuard {
        private:
            ParseContext& context;
            bool previousState;
        public:
            explicit AsyncContextGuard(ParseContext& ctx, bool async)
                : context(ctx), previousState(ctx.insideAsyncFunction) {
                context.insideAsyncFunction = async;
            }
            ~AsyncContextGuard() {
                context.insideAsyncFunction = previousState;
            }
            // Prevent copying
            AsyncContextGuard(const AsyncContextGuard&) = delete;
            AsyncContextGuard& operator=(const AsyncContextGuard&) = delete;
        };

        // Function context management
        /// @brief Check if we're currently inside a function body
        [[nodiscard]] bool isInsideFunctionBody() const { return insideFunctionBody; }

        /// @brief Set function body context
        void setFunctionContext(bool inFunction) { insideFunctionBody = inFunction; }

        /// @brief RAII helper for function context management
        class FunctionContextGuard {
        private:
            ParseContext& context;
            bool previousState;
        public:
            explicit FunctionContextGuard(ParseContext& ctx)
                : context(ctx), previousState(ctx.insideFunctionBody) {
                context.insideFunctionBody = true;
            }
            ~FunctionContextGuard() {
                context.insideFunctionBody = previousState;
            }
            // Prevent copying
            FunctionContextGuard(const FunctionContextGuard&) = delete;
            FunctionContextGuard& operator=(const FunctionContextGuard&) = delete;
        };

        // Class context management
        /// @brief Check if we're currently inside a class body
        [[nodiscard]] bool isInsideClassBody() const { return insideClassBody; }

        /// @brief Set class body context
        void setClassContext(bool inClass) { insideClassBody = inClass; }

        /// @brief RAII helper for class context management
        class ClassContextGuard {
        private:
            ParseContext& context;
            bool previousState;
        public:
            explicit ClassContextGuard(ParseContext& ctx)
                : context(ctx), previousState(ctx.insideClassBody) {
                context.insideClassBody = true;
            }
            ~ClassContextGuard() {
                context.insideClassBody = previousState;
            }
            // Prevent copying
            ClassContextGuard(const ClassContextGuard&) = delete;
            ClassContextGuard& operator=(const ClassContextGuard&) = delete;
        };

        // Interface context management
        /// @brief Check if we're currently inside an interface body
        [[nodiscard]] bool isInsideInterfaceBody() const { return insideInterfaceBody; }

        /// @brief Set interface body context
        void setInterfaceContext(bool inInterface) { insideInterfaceBody = inInterface; }

        /// @brief RAII helper for interface context management
        class InterfaceContextGuard {
        private:
            ParseContext& context;
            bool previousState;
        public:
            explicit InterfaceContextGuard(ParseContext& ctx)
                : context(ctx), previousState(ctx.insideInterfaceBody) {
                context.insideInterfaceBody = true;
            }
            ~InterfaceContextGuard() {
                context.insideInterfaceBody = previousState;
            }
            // Prevent copying
            InterfaceContextGuard(const InterfaceContextGuard&) = delete;
            InterfaceContextGuard& operator=(const InterfaceContextGuard&) = delete;
        };

        // Constructor context management
        /// @brief Check if we're currently inside a constructor body
        [[nodiscard]] bool isInsideConstructorBody() const { return insideConstructorBody; }

        /// @brief Set constructor body context
        void setConstructorContext(bool inConstructor) { insideConstructorBody = inConstructor; }

        /// @brief RAII helper for constructor context management
        class ConstructorContextGuard {
        private:
            ParseContext& context;
            bool previousState;
        public:
            explicit ConstructorContextGuard(ParseContext& ctx)
                : context(ctx), previousState(ctx.insideConstructorBody) {
                context.insideConstructorBody = true;
            }
            ~ConstructorContextGuard() {
                context.insideConstructorBody = previousState;
            }
            // Prevent copying
            ConstructorContextGuard(const ConstructorContextGuard&) = delete;
            ConstructorContextGuard& operator=(const ConstructorContextGuard&) = delete;
        };

        // NEW: Type name tracking for duplicate detection
        /// @brief Check if a type name (class or interface) has already been declared
        [[nodiscard]] bool isTypeDeclared(const std::string& typeName) const {
            return declaredTypeNames.count(typeName) > 0;
        }

        /// @brief Register a type name (class or interface) as declared
        void registerTypeName(const std::string& typeName) {
            declaredTypeNames.insert(typeName);
        }

        /// @brief Clear all declared type names (useful for new file/module parsing)
        void clearDeclaredTypes() {
            declaredTypeNames.clear();
            declaredClasses.clear();
            declaredInterfaces.clear();
            finalClasses.clear();
            finalInterfaces.clear();
            classParents.clear();
            interfaceParents.clear();
        }

        // NEW: Separate class/interface tracking for validation
        /// @brief Check if a name has been declared as a class
        [[nodiscard]] bool isClassDeclared(const std::string& className) const {
            return declaredClasses.count(className) > 0;
        }

        /// @brief Check if a name has been declared as an interface
        [[nodiscard]] bool isInterfaceDeclared(const std::string& interfaceName) const {
            return declaredInterfaces.count(interfaceName) > 0;
        }

        /// @brief Register a class name
        void registerClass(const std::string& className, bool isFinal = false) {
            declaredClasses.insert(className);
            declaredTypeNames.insert(className);
            if (isFinal) {
                finalClasses.insert(className);
            }
        }

        /// @brief Register an interface name
        void registerInterface(const std::string& interfaceName, bool isFinal = false) {
            declaredInterfaces.insert(interfaceName);
            declaredTypeNames.insert(interfaceName);
            if (isFinal) {
                finalInterfaces.insert(interfaceName);
            }
        }

        // NEW: Final modifier tracking
        /// @brief Check if a class is marked as final
        [[nodiscard]] bool isClassFinal(const std::string& className) const {
            return finalClasses.count(className) > 0;
        }

        /// @brief Check if an interface is marked as final
        [[nodiscard]] bool isInterfaceFinal(const std::string& interfaceName) const {
            return finalInterfaces.count(interfaceName) > 0;
        }

        // NEW: Circular inheritance detection
        /// @brief Register class inheritance relationship and check for cycles
        /// @return true if no cycle detected, false if circular inheritance found
        bool registerClassInheritance(const std::string& childClass, const std::string& parentClass) {
            // Extract base names without generic parameters
            std::string baseParent = parentClass;
            size_t genericStart = parentClass.find('<');
            if (genericStart != std::string::npos) {
                baseParent = parentClass.substr(0, genericStart);
            }

            // Check for immediate self-inheritance
            if (childClass == baseParent) {
                return false;
            }

            // Check for circular inheritance by traversing parent chain
            std::string current = baseParent;
            std::unordered_set<std::string> visited;
            visited.insert(childClass);

            while (classParents.count(current) > 0) {
                if (visited.count(current) > 0) {
                    // Cycle detected
                    return false;
                }
                visited.insert(current);
                current = classParents[current];

                // Extract base name from parent
                genericStart = current.find('<');
                if (genericStart != std::string::npos) {
                    current = current.substr(0, genericStart);
                }
            }

            // No cycle found, register the relationship
            classParents[childClass] = baseParent;
            return true;
        }

        /// @brief Register interface inheritance relationship and check for cycles
        /// @return true if no cycle detected, false if circular inheritance found
        bool registerInterfaceInheritance(const std::string& childInterface,
                                         const std::vector<std::string>& parentInterfaces) {
            // Extract base names without generic parameters
            std::vector<std::string> baseParents;
            for (const auto& parent : parentInterfaces) {
                std::string baseParent = parent;
                size_t genericStart = parent.find('<');
                if (genericStart != std::string::npos) {
                    baseParent = parent.substr(0, genericStart);
                }

                // Check for immediate self-inheritance
                if (childInterface == baseParent) {
                    return false;
                }

                baseParents.push_back(baseParent);
            }

            // Check for circular inheritance using DFS
            std::unordered_set<std::string> visited;
            std::unordered_set<std::string> recursionStack;

            auto hasCycle = [this, &visited, &recursionStack](const std::string& interface, auto& hasCycleRef) -> bool {
                if (recursionStack.count(interface) > 0) {
                    // Found cycle
                    return true;
                }

                if (visited.count(interface) > 0) {
                    // Already visited this path, no cycle
                    return false;
                }

                visited.insert(interface);
                recursionStack.insert(interface);

                // Check all parents of this interface
                if (interfaceParents.count(interface) > 0) {
                    for (const auto& parent : interfaceParents.at(interface)) {
                        if (hasCycleRef(parent, hasCycleRef)) {
                            return true;
                        }
                    }
                }

                recursionStack.erase(interface);
                return false;
            };

            // Check if adding these parents would create a cycle
            for (const auto& baseParent : baseParents) {
                visited.clear();
                recursionStack.clear();
                recursionStack.insert(childInterface);

                if (hasCycle(baseParent, hasCycle)) {
                    return false;
                }
            }

            // No cycle found, register the relationships
            interfaceParents[childInterface] = baseParents;
            return true;
        }

        /// @brief Get the inheritance chain for a class (for error messages)
        [[nodiscard]] std::vector<std::string> getClassInheritanceChain(const std::string& className) const {
            std::vector<std::string> chain;
            std::string current = className;

            while (classParents.count(current) > 0) {
                chain.push_back(current);
                current = classParents.at(current);
            }
            chain.push_back(current);  // Add final parent

            return chain;
        }

        // NEW: Global function name tracking for duplicate detection
        /// @brief Check if a global function name has already been declared
        [[nodiscard]] bool isFunctionDeclared(const std::string& functionName) const {
            return declaredFunctionNames.count(functionName) > 0;
        }

        /// @brief Register a global function name as declared
        void registerFunctionName(const std::string& functionName) {
            declaredFunctionNames.insert(functionName);
        }

        /// @brief Clear all declared function names (useful for new file/module parsing)
        void clearDeclaredFunctions() {
            declaredFunctionNames.clear();
        }
    };
}
