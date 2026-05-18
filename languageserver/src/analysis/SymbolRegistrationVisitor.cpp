#include "SymbolRegistrationVisitor.hpp"
#include "../DocumentManager.hpp"
#include "../../../mType/ast/nodes/statements/ProgramNode.hpp"
#include "../../../mType/ast/nodes/classes/ClassNode.hpp"
#include "../../../mType/ast/nodes/classes/MethodNode.hpp"
#include "../../../mType/ast/nodes/classes/FieldNode.hpp"
#include "../../../mType/ast/nodes/classes/InterfaceNode.hpp"
#include "../../../mType/ast/nodes/functions/FunctionNode.hpp"
#include "../../../mType/environment/registry/ClassDefinition.hpp"
#include "../../../mType/environment/registry/MethodDefinition.hpp"
#include "../../../mType/environment/registry/FieldDefinition.hpp"
#include "../../../mType/environment/registry/InterfaceDefinition.hpp"
#include "../../../mType/environment/registry/FunctionDefinition.hpp"
#include "../../../mType/value/ValueType.hpp"
#include "../../../mType/types/UnifiedType.hpp"

namespace mtype::lsp {

SymbolRegistrationVisitor::SymbolRegistrationVisitor(std::shared_ptr<environment::Environment> env)
    : environment_(env) {
}

void SymbolRegistrationVisitor::processProgram(ast::ASTNode* programNode, const std::string& uri) {
    currentUri_ = uri;

    if (!programNode) return;

    // Try to cast to ProgramNode
    auto* progNode = dynamic_cast<ast::nodes::statements::ProgramNode*>(programNode);
    if (!progNode) return;

    // Process all statements in the program
    for (const auto& statement : progNode->getStatements()) {
        if (statement) {
            processStatement(statement.get());
        }
    }
}

void SymbolRegistrationVisitor::processStatement(ast::ASTNode* node) {
    if (!node) return;

    // Check if it's a class node
    if (auto* classNode = dynamic_cast<ast::nodes::classes::ClassNode*>(node)) {
        processClassNode(classNode);
        return;
    }

    // Check if it's an interface node
    if (auto* interfaceNode = dynamic_cast<ast::nodes::classes::InterfaceNode*>(node)) {
        processInterfaceNode(interfaceNode);
        return;
    }

    // Check if it's a function node
    if (auto* functionNode = dynamic_cast<ast::nodes::functions::FunctionNode*>(node)) {
        processFunctionNode(functionNode);
        return;
    }
}

void SymbolRegistrationVisitor::processClassNode(ast::ASTNode* node) {
    auto* classNode = dynamic_cast<ast::nodes::classes::ClassNode*>(node);
    if (!classNode || !environment_) return;

    const auto& className = classNode->getClassName();

    // Register class in the environment's class registry
    if (environment_->getClassRegistry()) {
        try {
            // Create a basic class definition for the LSP
            auto classDef = std::make_shared<runtimeTypes::klass::ClassDefinition>(className);

            // Set final and abstract modifiers
            classDef->setFinal(classNode->isFinal());
            classDef->setAbstract(classNode->isAbstract());

            // Set parent class if exists
            if (classNode->hasParentClass()) {
                classDef->setParentClassName(classNode->getParentClassName());
            }

            // Register interfaces
            for (const auto& interfaceName : classNode->getImplementedInterfaces()) {
                classDef->addImplementedInterface(interfaceName);
            }

            // Register methods in the ClassDefinition
            for (const auto& method : classNode->getMethods()) {
                auto* methodNode = dynamic_cast<ast::nodes::classes::MethodNode*>(method.get());
                if (methodNode) {
                    const auto& methodName = methodNode->getName();
                    const auto& methodLoc = methodNode->getLocation();

                    // Create a MethodDefinition for LSP purposes
                    // For LSP, we only need basic type information, not full generic details
                    // Convert generic parameters to legacy format
                    std::vector<std::pair<std::string, value::ValueType>> legacyParams;
                    for (const auto& [paramName, genericType] : methodNode->getGenericParameters()) {
                        // Extract concrete ValueType from GenericType
                        value::ValueType vt = value::ValueType::VOID;
                        if (genericType && !genericType->isGenericParameter()) {
                            vt = genericType->getConcreteType();
                        }
                        legacyParams.emplace_back(paramName, vt);
                    }

                    auto methodDef = std::make_shared<runtimeTypes::klass::MethodDefinition>(
                        methodName,
                        methodNode->getReturnType(),
                        legacyParams,
                        methodNode->getBody(),
                        methodNode->getIsStatic(),
                        methodNode->getAccessModifier()
                    );

                    // Add to class definition
                    if (methodNode->getIsStatic()) {
                        classDef->addStaticMethod(methodName, methodDef);
                    } else {
                        classDef->addInstanceMethod(methodName, methodDef);
                    }

                    // Store method location with class context
                    // Key format: "ClassName.methodName"
                    std::string methodKey = className + "." + methodName;
                    symbolLocations_[methodKey] = SymbolLocationInfo{
                        currentUri_,
                        methodLoc.getLine() - 1,  // Convert to 0-based
                        methodLoc.getColumn() - 1,
                        className  // Track which class this method belongs to
                    };
                }
            }

            // Register fields in the ClassDefinition
            for (const auto& field : classNode->getFields()) {
                auto* fieldNode = dynamic_cast<ast::nodes::classes::FieldNode*>(field.get());
                if (fieldNode) {
                    const auto& fieldName = fieldNode->getName();
                    const auto& fieldLoc = fieldNode->getLocation();

                    // Create a FieldDefinition for LSP purposes
                    auto fieldDef = std::make_shared<runtimeTypes::klass::FieldDefinition>(
                        fieldName,
                        fieldNode->getType(),
                        value::Value{},  // Empty value for LSP (std::variant with monostate)
                        fieldNode->getIsStatic(),
                        fieldNode->getIsFinal(),
                        fieldNode->getAccessModifier()  // Use actual access modifier from field
                    );

                    // Add to class definition
                    if (fieldNode->getIsStatic()) {
                        classDef->addStaticField(fieldName, fieldDef);
                    } else {
                        classDef->addInstanceField(fieldName, fieldDef);
                    }

                    // Store field location with class context
                    // Key format: "ClassName.fieldName"
                    std::string fieldKey = className + "." + fieldName;
                    symbolLocations_[fieldKey] = SymbolLocationInfo{
                        currentUri_,
                        fieldLoc.getLine() - 1,  // Convert to 0-based
                        fieldLoc.getColumn() - 1,
                        className  // Track which class this field belongs to
                    };
                }
            }

            environment_->registerClass(className, classDef);

            // Track symbol location for the class
            const auto& location = classNode->getLocation();
            symbolLocations_[className] = SymbolLocationInfo{
                currentUri_,
                location.getLine() - 1,  // Convert 1-based to 0-based for LSP
                location.getColumn() - 1,  // Convert 1-based to 0-based for LSP
                ""  // Empty className for top-level symbols
            };

        } catch (const std::exception&) {
            // Silently ignore registration errors in LSP mode
        }
    }
}

void SymbolRegistrationVisitor::processInterfaceNode(ast::ASTNode* node) {
    auto* interfaceNode = dynamic_cast<ast::nodes::classes::InterfaceNode*>(node);
    if (!interfaceNode || !environment_) return;

    const auto& interfaceName = interfaceNode->getName();

    // Register interface in the environment's class registry
    if (environment_->getClassRegistry()) {
        try {
            // Create a basic interface definition for the LSP
            auto interfaceDef = std::make_shared<runtimeTypes::klass::InterfaceDefinition>(
                interfaceName
            );

            // Register parent interfaces
            for (const auto& parentInterface : interfaceNode->getExtendedInterfaces()) {
                interfaceDef->addExtendedInterface(parentInterface);
            }

            // Register method signatures from the interface
            for (const auto& method : interfaceNode->getMethods()) {
                // Cast to FunctionNode to extract method signature details
                auto* functionNode = dynamic_cast<ast::nodes::functions::FunctionNode*>(method.get());
                if (functionNode) {
                    runtimeTypes::klass::MethodSignature signature;
                    signature.name = functionNode->getName();

                    // Convert GenericType to UnifiedType for LSP compatibility
                    auto genericRetType = functionNode->getGenericReturnType();
                    if (genericRetType) {
                        signature.returnType = types::UnifiedType::classType(genericRetType->getBaseTypeName());
                    }

                    // Convert generic parameters to UnifiedType pairs
                    for (const auto& [paramName, paramGenericType] : functionNode->getGenericParameters()) {
                        types::UnifiedTypePtr paramType;
                        if (paramGenericType) {
                            paramType = types::UnifiedType::classType(paramGenericType->getBaseTypeName());
                        }
                        signature.parameters.emplace_back(paramName, paramType);
                    }

                    signature.genericParameters = functionNode->getGenericTypeParameters();

                    interfaceDef->addMethodSignature(signature);
                }
            }

            environment_->registerInterface(interfaceName, interfaceDef);

            // Track symbol location
            const auto& location = interfaceNode->getLocation();
            symbolLocations_[interfaceName] = SymbolLocationInfo{
                currentUri_,
                location.getLine() - 1,  // Convert 1-based to 0-based for LSP
                location.getColumn() - 1,  // Convert 1-based to 0-based for LSP
                ""  // Empty className for top-level symbols
            };

        } catch (const std::exception&) {
            // Silently ignore registration errors in LSP mode
        }
    }
}

void SymbolRegistrationVisitor::processFunctionNode(ast::ASTNode* node) {
    auto* functionNode = dynamic_cast<ast::nodes::functions::FunctionNode*>(node);
    if (!functionNode || !environment_) return;

    const auto& functionName = functionNode->getName();

    // Register function in the environment's function registry
    if (environment_->getFunctionRegistry()) {
        try {
            // Create a basic function definition for the LSP
            auto funcDef = std::make_shared<runtimeTypes::global::FunctionDefinition>(
                functionName,
                functionNode->getReturnType(),
                functionNode->getParameterTypes()
            );

            funcDef->setIsAsync(functionNode->getIsAsync());

            environment_->registerFunction(functionName, funcDef);

            // Track symbol location
            const auto& location = functionNode->getLocation();
            symbolLocations_[functionName] = SymbolLocationInfo{
                currentUri_,
                location.getLine() - 1,  // Convert 1-based to 0-based for LSP
                location.getColumn() - 1,  // Convert 1-based to 0-based for LSP
                ""  // Empty className for top-level symbols
            };

        } catch (const std::exception&) {
            // Silently ignore registration errors in LSP mode
        }
    }
}

} // namespace mtype::lsp
