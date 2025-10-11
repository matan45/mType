#include "ImportAndFunctionHandler.hpp"
#include "../ExpressionEvaluator.hpp"
#include "../StatementEvaluator.hpp"
#include "../../ast/nodes/statements/ImportNode.hpp"
#include "../../ast/nodes/statements/ProgramNode.hpp"
#include "../../ast/nodes/statements/NativeFunctionNode.hpp"
#include "../../ast/nodes/functions/FunctionNode.hpp"
#include "../../ast/nodes/classes/ClassNode.hpp"
#include "../../ast/nodes/classes/InterfaceNode.hpp"
#include "../../runtimeTypes/global/FunctionDefinition.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/klass/InterfaceDefinition.hpp"
#include "../../value/LambdaValue.hpp"
#include "../../constants/LambdaConstants.hpp"
#include "../../errors/TypeException.hpp"
#include "../../services/ImportManager.hpp"
#include "../../environment/registry/ExportRegistry.hpp"

using namespace errors;
using namespace runtimeTypes::global;
using namespace environment::registry;

namespace evaluator {
namespace statements {

    Value ImportAndFunctionHandler::evaluateImport(ImportNode* node)
    {
        auto env = context->getEnvironment();
        auto importManager = env->getImportManager();
        auto exportRegistry = env->getExportRegistry();

        if (!importManager)
        {
            throw TypeException("Import manager not available for import evaluation");
        }

        if (!exportRegistry)
        {
            throw TypeException("Export registry not available for import evaluation");
        }

        std::string filePath = node->getFilePath();
        std::string resolvedPath = importManager->resolvePath(filePath);

        // Check if already evaluated to avoid re-evaluation
        if (importManager->isEvaluated(resolvedPath))
        {
            // File already evaluated, but we still need to validate imports
            validateAndImportSymbols(node, resolvedPath, exportRegistry);
            return std::monostate{};
        }

        // Check for circular imports
        if (importManager->isBeingEvaluated(resolvedPath))
        {
            throw TypeException("Circular import detected: " + filePath + " is already being imported");
        }

        // Mark as being evaluated to prevent circular imports
        importManager->markAsBeingEvaluated(resolvedPath);
        std::string savedCurrentFile = importManager->getCurrentFilePath();

        try
        {
            // Set current file to the resolved path
            importManager->setCurrentFilePath(resolvedPath);

            // Parse the imported file
            ASTNode* importedAST = importManager->parseAndCacheAST(filePath);

            // STEP 1: First pass - collect all exported symbols from the file
            collectExportedSymbols(importedAST, resolvedPath, exportRegistry);

            // STEP 2: Validate that requested symbols exist and are public
            validateAndImportSymbols(node, resolvedPath, exportRegistry);

            // STEP 3: Evaluate the imported file (registers classes, functions, etc.)
            env->setImportEvaluation(true);

            if (stmtEvaluator && importedAST)
            {
                if (auto programNode = dynamic_cast<ast::nodes::statements::ProgramNode*>(importedAST))
                {
                    const auto& statements = programNode->getStatements();
                    for (const auto& stmt : statements)
                    {
                        stmtEvaluator->evaluate(stmt.get());
                    }
                }
                else
                {
                    stmtEvaluator->evaluate(importedAST);
                }
            }

            env->setImportEvaluation(false);

            // Restore state
            importManager->setCurrentFilePath(savedCurrentFile);
            importManager->markAsEvaluated(resolvedPath);
            importManager->unmarkAsBeingEvaluated(resolvedPath);

            return std::monostate{};
        }
        catch (...)
        {
            env->setImportEvaluation(false);
            importManager->setCurrentFilePath(savedCurrentFile);
            importManager->unmarkAsBeingEvaluated(resolvedPath);
            throw;
        }
    }

    void ImportAndFunctionHandler::collectExportedSymbols(ASTNode* ast,
                                                          const std::string& filePath,
                                                          std::shared_ptr<ExportRegistry> exportRegistry)
    {
        if (!ast) return;

        // Handle ProgramNode - traverse all statements
        if (auto programNode = dynamic_cast<ast::nodes::statements::ProgramNode*>(ast))
        {
            const auto& statements = programNode->getStatements();
            for (const auto& stmt : statements)
            {
                collectExportedSymbolsFromNode(stmt.get(), filePath, exportRegistry);
            }
        }
        else
        {
            collectExportedSymbolsFromNode(ast, filePath, exportRegistry);
        }
    }

    void ImportAndFunctionHandler::collectExportedSymbolsFromNode(ASTNode* node,
                                                                  const std::string& filePath,
                                                                  std::shared_ptr<ExportRegistry> exportRegistry)
    {
        using namespace ast::nodes::classes;
        using namespace ast::nodes::functions;

        if (!node) return;

        // Register class
        if (auto classNode = dynamic_cast<ClassNode*>(node))
        {
            exportRegistry->registerSymbol(
                filePath,
                classNode->getClassName(),
                ExportedSymbolType::CLASS,
                classNode->getVisibility()
            );
        }
        // Register interface
        else if (auto interfaceNode = dynamic_cast<InterfaceNode*>(node))
        {
            exportRegistry->registerSymbol(
                filePath,
                interfaceNode->getName(),
                ExportedSymbolType::INTERFACE,
                interfaceNode->getVisibility()
            );
        }
        // Register function
        else if (auto functionNode = dynamic_cast<FunctionNode*>(node))
        {
            exportRegistry->registerSymbol(
                filePath,
                functionNode->getName(),
                ExportedSymbolType::FUNCTION,
                functionNode->getVisibility()
            );
        }
        // Handle other node types if needed
    }

    void ImportAndFunctionHandler::validateAndImportSymbols(ImportNode* node,
                                                            const std::string& resolvedPath,
                                                            std::shared_ptr<ExportRegistry> exportRegistry)
    {
        if (node->isWildcard())
        {
            // Wildcard import - no validation needed, all public symbols are imported
            // The actual symbols are already registered in the environment
            return;
        }

        if (node->isSelective())
        {
            // Selective import - validate each symbol
            const auto& requestedSymbols = node->getImportedSymbols();

            for (const auto& symbolName : requestedSymbols)
            {
                // Check if symbol exists
                if (!exportRegistry->symbolExists(resolvedPath, symbolName))
                {
                    throw TypeException(
                        "Cannot import '" + symbolName + "' from '" + node->getFilePath() + "': " +
                        "Symbol not found. Available symbols: " + getAvailableSymbolsString(resolvedPath, exportRegistry)
                    );
                }

                // Check if symbol is public (exported)
                if (!exportRegistry->isSymbolExported(resolvedPath, symbolName))
                {
                    throw TypeException(
                        "Cannot import '" + symbolName + "' from '" + node->getFilePath() + "': " +
                        "Symbol is private and not exported. Only public symbols can be imported."
                    );
                }
            }
        }
    }

    std::string ImportAndFunctionHandler::getAvailableSymbolsString(const std::string& filePath,
                                                                    std::shared_ptr<ExportRegistry> exportRegistry)
    {
        auto publicSymbols = exportRegistry->getPublicSymbols(filePath);

        if (publicSymbols.empty())
        {
            return "(none)";
        }

        std::string result;
        for (size_t i = 0; i < publicSymbols.size(); ++i)
        {
            if (i > 0) result += ", ";
            result += publicSymbols[i];
            if (i >= 9 && publicSymbols.size() > 10)
            {
                result += ", ... (" + std::to_string(publicSymbols.size() - 10) + " more)";
                break;
            }
        }
        return result;
    }

    Value ImportAndFunctionHandler::evaluateFunction(FunctionNode* node)
    {
        auto env = context->getEnvironment();

        // Create function definition with interface support
        auto genericRetType = node->getGenericReturnType();
        std::string returnClassName = "";

        if (genericRetType && node->getReturnType() == ValueType::OBJECT)
        {
            // For object types, try to get the specific class/interface name
            std::string typeName = genericRetType->getBaseTypeName();
            if (typeName != "object")
            {
                returnClassName = typeName;
            }
            else if (genericRetType->isGenericParameter())
            {
                // It's a generic parameter, get its name
                returnClassName = genericRetType->getGenericName();
            }
        }

        // Fix parameter types to correctly mark interfaces vs classes
        auto rawParams = node->getParameterTypes();
        std::vector<std::pair<std::string, value::ParameterType>> correctedParams;

        for (const auto& [paramName, paramType] : rawParams)
        {
            value::ParameterType corrected = paramType;

            // If it's marked as a class, check if it's actually an interface
            if (paramType.isClass())
            {
                std::string typeName = paramType.getClassName();

                // Check if it's actually a registered interface
                if (env->findInterface(typeName) != nullptr)
                {
                    corrected = value::ParameterType::forInterface(typeName);
                }
            }

            correctedParams.emplace_back(paramName, corrected);
        }

        // Use ParameterType version to preserve class/interface information
        auto funcDef = std::make_shared<FunctionDefinition>(
            node->getName(),
            node->getReturnType(),
            correctedParams  // Use corrected parameter types
        );

        // Set the return class name if we found one
        if (!returnClassName.empty())
        {
            funcDef->setReturnClassName(returnClassName);
        }

        // Set the function body
        funcDef->setBody(node->getBody());

        // Set generic type parameters if the function is generic
        if (node->isGeneric())
        {
            funcDef->setGenericTypeParameters(node->getGenericTypeParameters());
        }

        // NEW: Set async flag if the function is async
        funcDef->setIsAsync(node->getIsAsync());

        // Register function in environment
        env->registerFunction(node->getName(), funcDef);


        return std::monostate{}; // Function definitions don't return values
    }

    Value ImportAndFunctionHandler::evaluateNativeFunction(ast::nodes::statements::NativeFunctionNode* node)
    {
        throw TypeException("Native function evaluation not implemented in refactored version");
    }

    Value ImportAndFunctionHandler::convertLambdaToInterface(const Value& lambdaValue,
                                                            const std::string& interfaceName,
                                                            const errors::SourceLocation& location)
    {
        // Extract the lambda value
        auto lambdaPtr = std::get<std::shared_ptr<value::LambdaValue>>(lambdaValue);
        auto* lambdaNode = lambdaPtr->getLambda();

        // Get the interface definition from the environment
        auto env = context->getEnvironment();
        auto interfaceDef = env->findInterface(interfaceName);

        if (!interfaceDef)
        {
            return lambdaValue;
        }

        // Check if the interface is functional (has exactly one method)
        if (!interfaceDef->isFunctionalInterface())
        {
            auto methodSignatures = interfaceDef->getMethodSignatures();
            throw errors::TypeException(
                "Cannot assign lambda to non-functional interface '" + interfaceName + "'. " +
                "Lambdas can only be assigned to interfaces with exactly one method. " +
                "Interface '" + interfaceName + "' has " + std::to_string(methodSignatures.size()) + " methods. " +
                "Consider using a functional interface (single method) or implement the interface explicitly.",
                location
            );
        }

        // Create the lambda implementation class
        auto implClass = interfaceDef->createLambdaImplementation(lambdaNode);
        if (!implClass)
        {
            return lambdaValue;
        }

        // Create an instance of the implementation class
        auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(implClass);

        // Store the lambda in a special field that the implementation can access
        instance->setField(constants::lambda::LAMBDA_FIELD_NAME, lambdaValue);

        return instance;
    }

} // namespace statements
} // namespace evaluator
