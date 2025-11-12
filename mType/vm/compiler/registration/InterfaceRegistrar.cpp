#include "InterfaceRegistrar.hpp"
#include "../../../ast/nodes/functions/FunctionNode.hpp"
#include "../../../ast/nodes/statements/ProgramNode.hpp"
#include "../../../ast/nodes/statements/BlockNode.hpp"
#include "../../../ast/nodes/statements/ImportNode.hpp"
#include "../../../ast/nodes/classes/ClassNode.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../errors/InheritanceException.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../runtimeTypes/klass/InterfaceDefinition.hpp"
#include "../../../ast/GenericType.hpp"
#include <stdexcept>
#include <algorithm>

namespace vm::compiler::registration
{
    // Helper function to normalize type strings for comparison
    // Removes spaces to handle formatting differences like "Function<String, Bool>" vs "Function<String,Bool>"
    static std::string normalizeTypeString(const std::string& typeStr)
    {
        std::string normalized;
        normalized.reserve(typeStr.size());
        for (char c : typeStr)
        {
            if (c != ' ')
            {
                normalized += c;
            }
        }
        return normalized;
    }
    InterfaceRegistrar::InterfaceRegistrar(
        std::shared_ptr<environment::Environment> environment,
        const types::GenericTypeResolver& genericResolver
    )
        : environment(environment)
        , genericResolver(genericResolver)
    {
    }

    void InterfaceRegistrar::registerInterfaceForBytecode(ast::nodes::classes::InterfaceNode* interfaceNode)
    {
        if (!interfaceNode) return;

        std::string interfaceName = interfaceNode->getName();

        // Check if interface already registered
        auto interfaceRegistry = environment->getInterfaceRegistry();
        if (!interfaceRegistry) {
            throw std::runtime_error("Interface registry not available");
        }

        if (interfaceRegistry->hasInterface(interfaceName)) {
            return; // Already registered, skip
        }

        // Validate generic parameter count (limit to 20)
        const auto& genericParams = interfaceNode->getGenericParameters();
        if (genericParams.size() > 20) {
            throw errors::TypeException(
                "Interface '" + interfaceName + "' has " + std::to_string(genericParams.size()) +
                " generic parameters, but the maximum allowed is 20",
                interfaceNode->getLocation()
            );
        }

        // Get extended interfaces
        const auto& extendedInterfaces = interfaceNode->getExtendedInterfaces();

        // Note: Parser already validates interface inheritance constraints at parse time:
        // - Interface cannot extend class (InterfaceParser.cpp:130-136)
        // - Cannot extend final interface (InterfaceParser.cpp:139-144)
        // - Circular interface inheritance (InterfaceParser.cpp:147-160)
        // This leaves only parent existence validation for cross-file imports

        // Validate that parent interfaces exist (needed for cross-file imports)
        for (const auto& parentInterface : extendedInterfaces) {
            // Extract base parent name from generic type
            std::string baseParentName = parentInterface;
            size_t genericStart = parentInterface.find('<');
            if (genericStart != std::string::npos) {
                baseParentName = parentInterface.substr(0, genericStart);
            }

            // Validate that parent interface exists
            if (!interfaceRegistry->hasInterface(baseParentName)) {
                throw errors::TypeException(
                    "Interface '" + interfaceName + "' extends undefined interface '" + parentInterface + "'",
                    interfaceNode->getLocation()
                );
            }
        }

        // Create interface definition with generics and extended interfaces
        auto interfaceDef = std::make_shared<runtimeTypes::klass::InterfaceDefinition>(
            interfaceName,
            genericParams,
            extendedInterfaces
        );

        // Set final modifier
        interfaceDef->setFinal(interfaceNode->isFinal());

        // Add method signatures
        for (const auto& method : interfaceNode->getMethods()) {
            if (auto* functionNode = dynamic_cast<ast::FunctionNode*>(method.get())) {
                runtimeTypes::klass::MethodSignature signature;
                signature.name = functionNode->getName();

                // Handle return type
                if (functionNode->getGenericReturnType()) {
                    signature.returnType = functionNode->getGenericReturnType();
                } else {
                    signature.returnType = std::make_shared<ast::GenericType>(
                        ::types::TypeConversionUtils::getTypeDisplayName(functionNode->getReturnType())
                    );
                }

                // Handle parameters
                const auto& genericParamPairs = functionNode->getGenericParameters();
                for (const auto& [paramName, paramType] : genericParamPairs) {
                    if (paramType) {
                        signature.parameters.emplace_back(paramName, paramType);
                    } else {
                        // Fallback if paramType is null (shouldn't happen in well-formed code)
                        signature.parameters.emplace_back(paramName, std::make_shared<ast::GenericType>(value::ValueType::VOID));
                    }
                }

                // Add generic parameters if any
                signature.genericParameters = functionNode->getGenericTypeParameters();

                interfaceDef->addMethodSignature(signature);
            }
        }

        // Register interface FIRST (needed for hierarchy validation)
        interfaceRegistry->registerInterface(interfaceName, interfaceDef);

        // Validate interface hierarchy (prevent circular inheritance)
        if (!interfaceRegistry->validateInterfaceHierarchy(interfaceName)) {
            // Unregister the interface since validation failed
            interfaceRegistry->removeInterface(interfaceName);
            throw errors::RuntimeException(
                "Circular interface inheritance detected for interface '" + interfaceName + "'",
                interfaceNode->getLocation()
            );
        }
    }

    void InterfaceRegistrar::validateInterfaceImplementations(
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
        ast::ClassNode* classNode
    )
    {
        auto interfaceRegistry = environment->getInterfaceRegistry();
        if (!interfaceRegistry) {
            return; // No interface registry, skip validation
        }

        // Use class location as fallback if classNode is not available
        const ast::SourceLocation classLocation = classNode ? classNode->getLocation() : ast::SourceLocation();

        for (const auto& interfaceName : classDef->getImplementedInterfaces())
        {
            // Parse interface name and extract generic type arguments
            auto [baseInterfaceName, typeArguments] = parseGenericInterfaceName(interfaceName);

            // Get interface definition
            auto interfaceDef = interfaceRegistry->findInterface(baseInterfaceName);
            if (!interfaceDef) {
                throw errors::TypeException(
                    "Interface '" + baseInterfaceName + "' not found",
                    classLocation
                );
            }

            const auto& interfaceGenericParams = interfaceDef->getGenericParameters();

            // Create type substitution map for generic resolution
            std::unordered_map<std::string, std::string> typeSubstitutions;

            if (typeArguments.size() != interfaceGenericParams.size()) {
                throw errors::TypeException(
                    "Interface '" + baseInterfaceName + "' expects " +
                    std::to_string(interfaceGenericParams.size()) + " type arguments but got " +
                    std::to_string(typeArguments.size()),
                    classLocation
                );
            }

            // Map generic parameters to concrete types
            for (size_t i = 0; i < interfaceGenericParams.size(); ++i) {
                typeSubstitutions[interfaceGenericParams[i].name] = typeArguments[i];
            }

            // Check that all interface methods are implemented
            const auto& methodSignatures = interfaceDef->getMethodSignatures();
            for (const auto& signature : methodSignatures) {
                auto method = classDef->getMethod(signature.name);

                // Find the method node to get its specific location
                ast::SourceLocation methodLocation = classLocation;
                if (classNode) {
                    const auto& methods = classNode->getMethods();
                    for (const auto& methodNodePtr : methods) {
                        if (auto methodNode = dynamic_cast<ast::nodes::classes::MethodNode*>(methodNodePtr.get())) {
                            if (methodNode->getName() == signature.name) {
                                methodLocation = methodNode->getLocation();
                                break;
                            }
                        }
                    }
                }

                if (!method) {
                    throw errors::TypeException(
                        "Class '" + classDef->getName() +
                        "' must implement method '" + signature.name +
                        "' from interface '" + interfaceName + "'",
                        methodLocation
                    );
                }

                // Validate that interface methods are implemented as public
                if (method->getAccessModifier() != ast::AccessModifier::PUBLIC) {
                    throw errors::TypeException(
                        "Method '" + signature.name + "' in class '" + classDef->getName() +
                        "' must be public when implementing interface '" + interfaceName + "'",
                        methodLocation
                    );
                }

                // Resolve return type with substitutions
                // Use toString() to get full type name including generic arguments
                std::string resolvedReturnType = resolveGenericType(
                    signature.returnType->toString(), typeSubstitutions);
                std::string methodReturnType;

                // Use generic return type if available, otherwise fall back to basic ValueType
                if (method->getGenericReturnType()) {
                    // Use toString() to include generic type arguments (e.g., "BinaryNode<T>" not just "BinaryNode")
                    methodReturnType = method->getGenericReturnType()->toString();
                } else {
                    methodReturnType = ::types::TypeConversionUtils::getTypeDisplayName(method->getReturnType());
                }

                // Normalize both type strings before comparison to handle spacing differences
                if (normalizeTypeString(methodReturnType) != normalizeTypeString(resolvedReturnType)) {
                    throw errors::TypeException(
                        "Method '" + signature.name + "' in class '" + classDef->getName() +
                        "' has return type '" + methodReturnType +
                        "' but interface '" + interfaceName + "' requires '" +
                        resolvedReturnType + "'",
                        methodLocation
                    );
                }

                // Validate parameter count matches (skip 'this' for instance methods)
                const auto& methodParams = method->getParameters();
                size_t userParamCount = methodParams.size();
                size_t paramOffset = 0;

                // For instance methods, skip the first parameter ('this')
                if (!method->isStatic() && !methodParams.empty()) {
                    userParamCount--;
                    paramOffset = 1;
                }

                if (userParamCount != signature.parameters.size()) {
                    throw errors::TypeException(
                        "Method '" + signature.name + "' in class '" + classDef->getName() +
                        "' has " + std::to_string(userParamCount) + " parameters" +
                        " but interface '" + interfaceName + "' requires " +
                        std::to_string(signature.parameters.size()) + " parameters",
                        methodLocation
                    );
                }

                // Validate parameter types with generic resolution
                const auto& methodGenericParams = method->getGenericParameters();
                for (size_t i = 0; i < signature.parameters.size(); ++i) {
                    std::string methodParamType;

                    // Use generic parameter type if available, otherwise fall back to basic ValueType
                    if (i < methodGenericParams.size() && methodGenericParams[i].second) {
                        // Use toString() to include generic type arguments (e.g., "BinaryNode<T>" not just "BinaryNode")
                        methodParamType = methodGenericParams[i].second->toString();
                    } else {
                        // Access method parameter at offset position (skip 'this' for instance methods)
                        methodParamType = ::types::TypeConversionUtils::getTypeDisplayName(methodParams[i + paramOffset].second);
                    }

                    // Use toString() to get full type name including generic arguments
                    std::string resolvedParamType = resolveGenericType(
                        signature.parameters[i].second->toString(), typeSubstitutions);

                    // Normalize both type strings before comparison to handle spacing differences
                    if (normalizeTypeString(methodParamType) != normalizeTypeString(resolvedParamType)) {
                        throw errors::TypeException(
                            "Method '" + signature.name + "' parameter " + std::to_string(i + 1) +
                            " in class '" + classDef->getName() +
                            "' has type '" + methodParamType +
                            "' but interface '" + interfaceName + "' requires '" +
                            resolvedParamType + "'",
                            methodLocation
                        );
                    }
                }
            }
        }
    }

    std::pair<std::string, std::vector<std::string>> InterfaceRegistrar::parseGenericInterfaceName(
        const std::string& interfaceName
    ) const
    {
        return genericResolver.parseGenericTypeName(interfaceName);
    }

    std::string InterfaceRegistrar::resolveGenericType(
        const std::string& typeName,
        const std::unordered_map<std::string, std::string>& substitutions
    ) const
    {
        return genericResolver.resolveGenericType(typeName, substitutions);
    }

    void InterfaceRegistrar::registerInterfaces(ast::ASTNode* node)
    {
        if (!node) return;

        // Check if this node is an InterfaceNode
        if (auto interfaceNode = dynamic_cast<ast::nodes::classes::InterfaceNode*>(node))
        {
            registerInterfaceForBytecode(interfaceNode);
            return; // No need to traverse children
        }

        // Recursively process child nodes
        if (auto programNode = dynamic_cast<ast::ProgramNode*>(node))
        {
            for (const auto& statement : programNode->getStatements())
            {
                registerInterfaces(statement.get());
            }
        }
        else if (auto blockNode = dynamic_cast<ast::BlockNode*>(node))
        {
            for (const auto& statement : blockNode->getStatements())
            {
                registerInterfaces(statement.get());
            }
        }
        else if (auto importNode = dynamic_cast<ast::nodes::statements::ImportNode*>(node))
        {
            if (importNode->isResolved() && importNode->getImportedAST())
            {
                registerInterfaces(importNode->getImportedAST());
            }
        }
    }
}
