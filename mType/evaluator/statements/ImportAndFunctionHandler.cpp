#include "ImportAndFunctionHandler.hpp"
#include "../ExpressionEvaluator.hpp"
#include "../StatementEvaluator.hpp"
#include "../../ast/nodes/statements/ImportNode.hpp"
#include "../../ast/nodes/statements/ProgramNode.hpp"
#include "../../ast/nodes/statements/NativeFunctionNode.hpp"
#include "../../ast/nodes/functions/FunctionNode.hpp"
#include "../../runtimeTypes/global/FunctionDefinition.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/klass/InterfaceDefinition.hpp"
#include "../../value/LambdaValue.hpp"
#include "../../constants/LambdaConstants.hpp"
#include "../../errors/TypeException.hpp"
#include "../../services/ImportManager.hpp"

using namespace errors;
using namespace runtimeTypes::global;

namespace evaluator {
namespace statements {

    Value ImportAndFunctionHandler::evaluateImport(ImportNode* node)
    {
        // Check if import is already resolved (e.g., from JSON deserialization)
        if (node->isResolved() && node->getImportedAST())
        {
            // Directly evaluate the embedded AST instead of resolving from files
            ASTNode* importedAST = node->getImportedAST();

            // Evaluate the embedded AST directly using the current evaluation context
            // This should register all classes, functions, etc. from the import
            if (importedAST && stmtEvaluator)
            {
                // Recursively evaluate the imported AST using the statement evaluator
                if (auto programNode = dynamic_cast<ast::nodes::statements::ProgramNode*>(importedAST))
                {
                    const auto& statements = programNode->getStatements();
                    for (const auto& stmt : statements)
                    {
                        // Delegate to statement evaluator
                        // NOTE: This calls evaluate which will dispatch to the correct handler
                        stmtEvaluator->evaluate(stmt.get());
                    }
                }
                else
                {
                    // Not a program node, evaluate directly
                    stmtEvaluator->evaluate(importedAST);
                }
            }

            return std::monostate{}; // Import completed successfully
        }

        // Fall back to normal ImportManager-based resolution for non-resolved imports
        auto env = context->getEnvironment();
        auto importManager = env->getImportManager();

        if (!importManager)
        {
            throw TypeException("Import manager not available for import evaluation");
        }

        std::string filePath = node->getFilePath();

        // Use appropriate path for resolution based on execution mode
        std::string resolvedPath = importManager->resolvePath(filePath);

        // Check if already evaluated to avoid re-evaluation
        if (importManager->isEvaluated(resolvedPath))
        {
            return std::monostate{}; // Already imported
        }

        // Check for circular imports
        if (importManager->isBeingEvaluated(resolvedPath))
        {
            throw TypeException("Circular import detected: " + filePath + " is already being imported");
        }

        // Mark as being evaluated to prevent circular imports
        importManager->markAsBeingEvaluated(resolvedPath);

        try
        {
            // Normal mode: Parse .mt file
            ASTNode* importedAST = importManager->parseAndCacheAST(filePath);

            // Set import evaluation context and evaluate the loaded AST
            env->setImportEvaluation(true);

            // Evaluate the imported AST
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

            // Mark as evaluated and no longer being evaluated
            importManager->markAsEvaluated(resolvedPath);
            importManager->unmarkAsBeingEvaluated(resolvedPath);

            return std::monostate{}; // Imports return void
        }
        catch (...)
        {
            env->setImportEvaluation(false);
            importManager->unmarkAsBeingEvaluated(resolvedPath);
            throw;
        }
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

        // Use ParameterType version to preserve class/interface information
        auto funcDef = std::make_shared<FunctionDefinition>(
            node->getName(),
            node->getReturnType(),
            node->getParameterTypes()  // Use getParameterTypes() instead of getParameters()
        );

        // Set the return class name if we found one
        if (!returnClassName.empty())
        {
            funcDef->setReturnClassName(returnClassName);
        }

        // Set the function body
        funcDef->setBody(node->getBody());

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
