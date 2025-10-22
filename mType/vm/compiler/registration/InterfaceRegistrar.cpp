#include "InterfaceRegistrar.hpp"
#include "../../../ast/nodes/functions/FunctionNode.hpp"
#include "../../../ast/nodes/statements/ProgramNode.hpp"
#include "../../../ast/nodes/statements/BlockNode.hpp"
#include "../../../ast/nodes/statements/ImportNode.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../errors/InheritanceException.hpp"
#include "../../runtime/utils/TypeConverter.hpp"
#include "../../../runtimeTypes/klass/InterfaceDefinition.hpp"
#include "../../../ast/GenericType.hpp"
#include <stdexcept>

namespace vm::compiler::registration
{
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
                        vm::runtime::utils::TypeConverter::valueTypeToString(functionNode->getReturnType())
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
        const ast::SourceLocation& location
    )
    {
        auto interfaceRegistry = environment->getInterfaceRegistry();
        if (!interfaceRegistry) {
            return; // No interface registry, skip validation
        }

        for (const auto& interfaceName : classDef->getImplementedInterfaces())
        {
            // Parse interface name and extract generic type arguments
            auto [baseInterfaceName, typeArguments] = parseGenericInterfaceName(interfaceName);

            // Get interface definition
            auto interfaceDef = interfaceRegistry->findInterface(baseInterfaceName);
            if (!interfaceDef) {
                throw errors::TypeException(
                    "Interface '" + baseInterfaceName + "' not found",
                    location
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
                    location
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
                if (!method) {
                    throw errors::TypeException(
                        "Class '" + classDef->getName() +
                        "' must implement method '" + signature.name +
                        "' from interface '" + interfaceName + "'",
                        location
                    );
                }

                // Resolve return type with substitutions
                std::string resolvedReturnType = resolveGenericType(
                    signature.returnType->getBaseTypeName(), typeSubstitutions);
                std::string methodReturnType;

                // Use generic return type if available, otherwise fall back to basic ValueType
                if (method->getGenericReturnType()) {
                    methodReturnType = method->getGenericReturnType()->getBaseTypeName();
                } else {
                    methodReturnType = vm::runtime::utils::TypeConverter::valueTypeToString(method->getReturnType());
                }

                if (methodReturnType != resolvedReturnType) {
                    throw errors::TypeException(
                        "Method '" + signature.name + "' in class '" + classDef->getName() +
                        "' has return type '" + methodReturnType +
                        "' but interface '" + interfaceName + "' requires '" +
                        resolvedReturnType + "'",
                        location
                    );
                }

                // Validate parameter count matches
                if (method->getParameters().size() != signature.parameters.size()) {
                    throw errors::TypeException(
                        "Method '" + signature.name + "' in class '" + classDef->getName() +
                        "' has " + std::to_string(method->getParameters().size()) + " parameters" +
                        " but interface '" + interfaceName + "' requires " +
                        std::to_string(signature.parameters.size()) + " parameters",
                        location
                    );
                }

                // Validate parameter types with generic resolution
                const auto& methodParams = method->getParameters();
                const auto& methodGenericParams = method->getGenericParameters();
                for (size_t i = 0; i < signature.parameters.size(); ++i) {
                    std::string methodParamType;

                    // Use generic parameter type if available, otherwise fall back to basic ValueType
                    if (i < methodGenericParams.size() && methodGenericParams[i].second) {
                        methodParamType = methodGenericParams[i].second->getBaseTypeName();
                    } else {
                        methodParamType = vm::runtime::utils::TypeConverter::valueTypeToString(methodParams[i].second);
                    }

                    std::string resolvedParamType = resolveGenericType(
                        signature.parameters[i].second->getBaseTypeName(), typeSubstitutions);

                    if (methodParamType != resolvedParamType) {
                        throw errors::TypeException(
                            "Method '" + signature.name + "' parameter " + std::to_string(i + 1) +
                            " in class '" + classDef->getName() +
                            "' has type '" + methodParamType +
                            "' but interface '" + interfaceName + "' requires '" +
                            resolvedParamType + "'",
                            location
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
