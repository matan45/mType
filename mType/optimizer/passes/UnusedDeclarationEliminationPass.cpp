/**
 * Unused Declaration Elimination Pass (O2 Optimization)
 *
 * This pass removes unused functions, classes, interfaces, and variables
 * while preserving entry point code and public/exported declarations.
 *
 * Implementation Strategy:
 * 1. Phase 1: Analyze entire AST to track declarations and usages
 * 2. Compute transitive closure (if func A calls func B, and A is used, then B is used)
 * 3. Phase 2: Remove declarations that are not used
 */

#include "UnusedDeclarationEliminationPass.hpp"
#include "../base/OptimizationContext.hpp"
#include "../OptimizationResult.hpp"
#include "../../ast/nodes/statements/ProgramNode.hpp"
#include "../../ast/nodes/statements/BlockNode.hpp"
#include "../../ast/nodes/statements/AssignmentNode.hpp"
#include "../../ast/nodes/statements/IfNode.hpp"
#include "../../ast/nodes/statements/WhileNode.hpp"
#include "../../ast/nodes/statements/ForNode.hpp"
#include "../../ast/nodes/statements/ForEachNode.hpp"
#include "../../ast/nodes/functions/FunctionNode.hpp"
#include "../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../ast/nodes/classes/ClassNode.hpp"
#include "../../ast/nodes/classes/FieldNode.hpp"
#include "../../ast/nodes/classes/MethodNode.hpp"
#include "../../ast/nodes/classes/MethodCallNode.hpp"
#include "../../ast/nodes/classes/NewNode.hpp"
#include "../../ast/nodes/classes/InterfaceNode.hpp"
#include "../../ast/AccessModifier.hpp"
#include "../../ast/nodes/expressions/VariableNode.hpp"
#include "../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../../ast/nodes/expressions/TernaryExpNode.hpp"
#include "../../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../../ast/nodes/expressions/ArrayCreationNode.hpp"
#include "../../ast/nodes/expressions/ArrayLiteralNode.hpp"
#include "../../ast/nodes/functions/ReturnNode.hpp"
#include "../../ast/nodes/statements/SwitchNode.hpp"
#include "../../ast/nodes/statements/CaseNode.hpp"
#include "../../ast/nodes/statements/ImportNode.hpp"
#include <chrono>
#include <iostream>

namespace optimizer::passes
{
    using namespace ast;
    using namespace ast::nodes::statements;
    using namespace ast::nodes::functions;
    using namespace ast::nodes::classes;
    using namespace ast::nodes::expressions;

    // Debug flag - SET TO TRUE to see detailed optimization logs
    constexpr bool UDE_DEBUG = false;

    // ================= UsageAnalyzer Implementation =================

    void UnusedDeclarationEliminationPass::UsageAnalyzer::analyzeDeclaredFunction(
        const std::string& name, bool isPublic)
    {
        if (UDE_DEBUG)
        {
            std::cout << "[UDE] Declaring function: " << name << (isPublic ? " (PUBLIC)" : " (PRIVATE)") << "\n";
        }
        declaredFunctions.insert(name);
        if (isPublic)
        {
            publicFunctions.insert(name);
            // NOTE: We DON'T mark public functions as automatically used
            // The optimizer will remove them if they're truly unused
            // usedFunctions.insert(name); // Public = always used
        }
    }

    void UnusedDeclarationEliminationPass::UsageAnalyzer::analyzeDeclaredClass(
        const std::string& name, bool isPublic)
    {
        if (UDE_DEBUG)
        {
            std::cout << "[UDE] Declaring class: " << name << (isPublic ? " (PUBLIC)" : " (PRIVATE)") << "\n";
        }
        declaredClasses.insert(name);
        if (isPublic)
        {
            publicClasses.insert(name);
            // NOTE: We DON'T mark public classes as automatically used
            // The optimizer will remove them if they're truly unused
            // usedClasses.insert(name); // Public = always used
        }
    }

    void UnusedDeclarationEliminationPass::UsageAnalyzer::analyzeDeclaredInterface(
        const std::string& name, bool isPublic)
    {
        declaredInterfaces.insert(name);
        if (isPublic)
        {
            publicInterfaces.insert(name);
            // NOTE: We DON'T mark public interfaces as automatically used
            // The optimizer will remove them if they're truly unused
            // usedInterfaces.insert(name); // Public = always used
        }
    }

    void UnusedDeclarationEliminationPass::UsageAnalyzer::analyzeDeclaredVariable(
        const std::string& name)
    {
        declaredVariables.insert(name);
    }

    void UnusedDeclarationEliminationPass::UsageAnalyzer::analyzeDeclaredMethod(
        const std::string& className, const std::string& methodName, bool isPublic, bool isStatic)
    {
        // Create unique key that includes static/instance distinction
        std::string fullName = className + "::" + (isStatic ? "STATIC::" : "") + methodName;

        if (UDE_DEBUG)
        {
            std::cout << "[UDE] Declaring method: " << fullName
                      << (isStatic ? " (STATIC)" : " (INSTANCE)")
                      << (isPublic ? " (PUBLIC)" : " (PRIVATE)") << "\n";
        }

        declaredMethods.insert(fullName);

        // Check if this is a special method that should always be kept
        if (methodName == "constructor" || methodName == "toString" ||
            methodName == "equals" || methodName == "hashCode")
        {
            specialMethods.insert(methodName);
            usedMethods.insert(fullName);  // Always mark special methods as used
            if (UDE_DEBUG)
            {
                std::cout << "[UDE]   Special method always kept: " << fullName << "\n";
            }
        }
        else if (isPublic)
        {
            publicMethods.insert(fullName);
            // NOTE: We DON'T automatically mark public methods as used
            // The optimizer will remove them if they're truly unused
        }
    }

    void UnusedDeclarationEliminationPass::UsageAnalyzer::analyzeUsedFunction(
        const std::string& name)
    {
        if (UDE_DEBUG)
        {
            std::cout << "[UDE] Marking function as USED: " << name << "\n";
        }
        usedFunctions.insert(name);
    }

    void UnusedDeclarationEliminationPass::UsageAnalyzer::analyzeUsedClass(
        const std::string& name)
    {
        if (UDE_DEBUG)
        {
            std::cout << "[UDE] Marking class as USED: " << name << "\n";
        }
        usedClasses.insert(name);
    }

    void UnusedDeclarationEliminationPass::UsageAnalyzer::analyzeUsedInterface(
        const std::string& name)
    {
        usedInterfaces.insert(name);
    }

    void UnusedDeclarationEliminationPass::UsageAnalyzer::analyzeUsedVariable(
        const std::string& name)
    {
        usedVariables.insert(name);
    }

    void UnusedDeclarationEliminationPass::UsageAnalyzer::analyzeUsedMethod(
        const std::string& className, const std::string& methodName)
    {
        // For simplicity, mark both static and instance versions as used
        // This is conservative but safe
        std::string instanceName = className + "::" + methodName;
        std::string staticName = className + "::STATIC::" + methodName;

        if (UDE_DEBUG)
        {
            std::cout << "[UDE] Marking method as USED: " << instanceName << " (and static variant)\n";
        }
        usedMethods.insert(instanceName);
        usedMethods.insert(staticName);
    }

    void UnusedDeclarationEliminationPass::UsageAnalyzer::computeTransitiveClosure()
    {
        // Compute transitive closure of function calls
        // If function A is used and calls function B, then B is also used
        bool changed = true;
        while (changed)
        {
            changed = false;
            for (const auto& usedFunc : usedFunctions)
            {
                auto it = functionCalls.find(usedFunc);
                if (it != functionCalls.end())
                {
                    for (const auto& calledFunc : it->second)
                    {
                        if (usedFunctions.find(calledFunc) == usedFunctions.end())
                        {
                            usedFunctions.insert(calledFunc);
                            changed = true;
                        }
                    }
                }
            }
        }
    }

    bool UnusedDeclarationEliminationPass::UsageAnalyzer::isFunctionUsed(
        const std::string& name) const
    {
        // Only check if actually used, not just public
        return usedFunctions.find(name) != usedFunctions.end();
    }

    bool UnusedDeclarationEliminationPass::UsageAnalyzer::isClassUsed(
        const std::string& name) const
    {
        // Only check if actually used, not just public
        return usedClasses.find(name) != usedClasses.end();
    }

    bool UnusedDeclarationEliminationPass::UsageAnalyzer::isInterfaceUsed(
        const std::string& name) const
    {
        // Only check if actually used, not just public
        return usedInterfaces.find(name) != usedInterfaces.end();
    }

    bool UnusedDeclarationEliminationPass::UsageAnalyzer::isVariableUsed(
        const std::string& name) const
    {
        return usedVariables.find(name) != usedVariables.end();
    }

    bool UnusedDeclarationEliminationPass::UsageAnalyzer::isMethodUsed(
        const std::string& className, const std::string& methodName) const
    {
        // Check both static and instance versions
        std::string instanceName = className + "::" + methodName;
        std::string staticName = className + "::STATIC::" + methodName;
        return usedMethods.find(instanceName) != usedMethods.end() ||
               usedMethods.find(staticName) != usedMethods.end();
    }

    // ================= AST Analysis (Phase 1) =================

    void UnusedDeclarationEliminationPass::analyzeAST(
        ast::ASTNode* node, UsageAnalyzer& analyzer)
    {
        if (!node) return;

        // Track declarations - but DON'T analyze their bodies yet
        // We'll analyze bodies only for used declarations in a second pass
        if (auto* funcNode = dynamic_cast<FunctionNode*>(node))
        {
            bool isPublic = funcNode->isPublic();
            analyzer.analyzeDeclaredFunction(funcNode->getName(), isPublic);
            // Don't analyze body here - we'll do it in second pass for used functions only
            return;
        }

        if (auto* classNode = dynamic_cast<ClassNode*>(node))
        {
            bool isPublic = classNode->isPublic();
            std::string className = classNode->getClassName();
            analyzer.analyzeDeclaredClass(className, isPublic);

            // Classes that extend other classes use those classes
            if (!classNode->getParentClassName().empty())
            {
                analyzer.analyzeUsedClass(classNode->getParentClassName());
            }

            // Classes that implement interfaces use those interfaces
            for (const auto& iface : classNode->getImplementedInterfaces())
            {
                analyzer.analyzeUsedInterface(iface);
            }

            // Declare all methods in this class
            for (const auto& method : classNode->getMethods())
            {
                if (auto* methodNode = dynamic_cast<MethodNode*>(method.get()))
                {
                    bool isMethodPublic = (methodNode->getAccessModifier() == AccessModifier::PUBLIC);
                    bool isStatic = methodNode->getIsStatic();
                    analyzer.analyzeDeclaredMethod(className, methodNode->getName(), isMethodPublic, isStatic);
                }
            }

            // Don't analyze method bodies here - we'll do it in second pass for used classes only
            return;
        }

        if (auto* ifaceNode = dynamic_cast<InterfaceNode*>(node))
        {
            bool isPublic = ifaceNode->isPublic();
            analyzer.analyzeDeclaredInterface(ifaceNode->getName(), isPublic);
            return;
        }

        if (auto* assignNode = dynamic_cast<AssignmentNode*>(node))
        {
            // AssignmentNode can be either a declaration or an assignment
            // We track it as a variable either way
            analyzer.analyzeDeclaredVariable(assignNode->getVariableName());

            // Analyze value/initializer for usages
            if (assignNode->getValue())
            {
                analyzeAST(assignNode->getValue(), analyzer);
            }
            return;
        }

        // Track usages
        if (auto* funcCallNode = dynamic_cast<FunctionCallNode*>(node))
        {
            analyzer.analyzeUsedFunction(funcCallNode->getFunctionName());

            // Analyze arguments
            for (const auto& arg : funcCallNode->getArguments())
            {
                analyzeAST(arg.get(), analyzer);
            }
            return;
        }

        if (auto* newNode = dynamic_cast<NewNode*>(node))
        {
            analyzer.analyzeUsedClass(newNode->getClassName());

            // Analyze constructor arguments
            for (const auto& arg : newNode->getArguments())
            {
                analyzeAST(arg.get(), analyzer);
            }
            return;
        }

        if (auto* varNode = dynamic_cast<VariableNode*>(node))
        {
            analyzer.analyzeUsedVariable(varNode->getName());
            return;
        }

        // Recursively analyze children
        if (auto* programNode = dynamic_cast<ProgramNode*>(node))
        {
            if (UDE_DEBUG)
            {
                std::cout << "[UDE] Analyzing ProgramNode with " << programNode->getStatements().size() << " statements\n";
            }
            for (const auto& stmt : programNode->getStatements())
            {
                analyzeAST(stmt.get(), analyzer);
            }
            return;
        }

        // Track ImportNodes (for debugging)
        if (auto* importNode = dynamic_cast<ImportNode*>(node))
        {
            if (UDE_DEBUG)
            {
                std::cout << "[UDE] Found ImportNode: " << importNode->getFilePath()
                          << " (type: " << (importNode->isWildcard() ? "wildcard" : "selective") << ")\n";
            }

            // Analyze the imported AST directly (set by resolveImports)
            if (importNode->getImportedAST())
            {
                if (UDE_DEBUG)
                {
                    std::cout << "[UDE] Analyzing imported AST from: " << importNode->getFilePath() << "\n";
                }
                analyzeAST(importNode->getImportedAST(), analyzer);
            }
            else if (UDE_DEBUG)
            {
                std::cout << "[UDE] WARNING: ImportNode has no imported AST: " << importNode->getFilePath() << "\n";
            }

            return;
        }

        if (auto* blockNode = dynamic_cast<BlockNode*>(node))
        {
            for (const auto& stmt : blockNode->getStatements())
            {
                analyzeAST(stmt.get(), analyzer);
            }
            return;
        }

        if (auto* ifNode = dynamic_cast<IfNode*>(node))
        {
            analyzeAST(ifNode->getCondition(), analyzer);
            analyzeAST(ifNode->getThenStatement(), analyzer);
            if (ifNode->hasElseStatement())
            {
                analyzeAST(ifNode->getElseStatement(), analyzer);
            }
            return;
        }

        if (auto* whileNode = dynamic_cast<WhileNode*>(node))
        {
            analyzeAST(whileNode->getCondition(), analyzer);
            analyzeAST(whileNode->getBody(), analyzer);
            return;
        }

        if (auto* forNode = dynamic_cast<ForNode*>(node))
        {
            analyzeAST(forNode->getInitialization(), analyzer);
            analyzeAST(forNode->getCondition(), analyzer);
            analyzeAST(forNode->getUpdate(), analyzer);
            analyzeAST(forNode->getBody(), analyzer);
            return;
        }

        if (auto* foreachNode = dynamic_cast<ForEachNode*>(node))
        {
            analyzeAST(foreachNode->getCollection(), analyzer);
            analyzeAST(foreachNode->getBody(), analyzer);
            return;
        }

        if (auto* switchNode = dynamic_cast<SwitchNode*>(node))
        {
            analyzeAST(switchNode->getExpression(), analyzer);
            for (const auto& caseNode : switchNode->getCases())
            {
                analyzeAST(caseNode.get(), analyzer);
            }
            return;
        }

        if (auto* caseNode = dynamic_cast<CaseNode*>(node))
        {
            analyzeAST(caseNode->getValue(), analyzer);
            for (const auto& stmt : caseNode->getStatements())
            {
                analyzeAST(stmt.get(), analyzer);
            }
            return;
        }

        if (auto* methodCallNode = dynamic_cast<MethodCallNode*>(node))
        {
            std::string methodName = methodCallNode->getMethodName();

            // For static calls (Class::method), try to extract the class name
            if (methodCallNode->getIsStaticCall())
            {
                // The object is a VariableNode containing the class name
                if (auto* classNameNode = dynamic_cast<VariableNode*>(methodCallNode->getObject()))
                {
                    std::string className = classNameNode->getName();
                    // Mark static method explicitly
                    std::string staticMethodKey = className + "::STATIC::" + methodName;
                    analyzer.usedMethods.insert(staticMethodKey);
                    if (UDE_DEBUG)
                    {
                        std::cout << "[UDE] Found static method call: " << staticMethodKey << "\n";
                    }
                }
            }
            else
            {
                // For instance calls, track method name only
                analyzer.calledMethodNames.insert(methodName);
                if (UDE_DEBUG)
                {
                    std::cout << "[UDE] Found instance method call: " << methodName << "()\n";
                }
            }

            // Analyze the object being called on
            analyzeAST(methodCallNode->getObject(), analyzer);

            // Analyze all arguments
            for (const auto& arg : methodCallNode->getArguments())
            {
                analyzeAST(arg.get(), analyzer);
            }
            return;
        }

        if (auto* returnNode = dynamic_cast<ReturnNode*>(node))
        {
            if (returnNode->hasReturnValue())
            {
                analyzeAST(returnNode->getReturnValue(), analyzer);
            }
            return;
        }

        // Handle binary expressions (e.g., "a + b")
        if (auto* binaryNode = dynamic_cast<ast::nodes::expressions::BinaryExpNode*>(node))
        {
            analyzeAST(binaryNode->getLeft(), analyzer);
            analyzeAST(binaryNode->getRight(), analyzer);
            return;
        }

        // Handle unary expressions (e.g., "!x", "-y")
        if (auto* unaryNode = dynamic_cast<ast::nodes::expressions::UnaryExpNode*>(node))
        {
            analyzeAST(unaryNode->getOperand(), analyzer);
            return;
        }

        // Handle ternary expressions (e.g., "condition ? a : b")
        if (auto* ternaryNode = dynamic_cast<ast::nodes::expressions::TernaryExpNode*>(node))
        {
            analyzeAST(ternaryNode->getCondition(), analyzer);
            analyzeAST(ternaryNode->getTrueExpression(), analyzer);
            analyzeAST(ternaryNode->getFalseExpression(), analyzer);
            return;
        }

        // Handle index access (e.g., "array[0]")
        if (auto* indexNode = dynamic_cast<ast::nodes::expressions::IndexAccessNode*>(node))
        {
            analyzeAST(indexNode->getCollection(), analyzer);
            analyzeAST(indexNode->getIndex(), analyzer);
            return;
        }

        // Handle array creation (e.g., "new int[10]")
        if (auto* arrayCreationNode = dynamic_cast<ast::nodes::expressions::ArrayCreationNode*>(node))
        {
            // Analyze size expressions if present
            for (const auto& sizeExpr : arrayCreationNode->getSizeExpressions())
            {
                analyzeAST(sizeExpr.get(), analyzer);
            }
            return;
        }

        // Handle array literals (e.g., "[1, 2, 3]")
        if (auto* arrayLiteralNode = dynamic_cast<ast::nodes::expressions::ArrayLiteralNode*>(node))
        {
            // Analyze each element in the array literal
            for (const auto& element : arrayLiteralNode->getElements())
            {
                analyzeAST(element.get(), analyzer);
            }
            return;
        }

        // For any other node types, we don't need to analyze them for this pass
        // (literals, etc. don't declare or use functions/classes)
    }

    void UnusedDeclarationEliminationPass::analyzeUsedDeclarations(
        ast::ASTNode* node, UsageAnalyzer& analyzer)
    {
        if (!node) return;

        // Only analyze bodies of functions/classes that are marked as used
        if (auto* funcNode = dynamic_cast<FunctionNode*>(node))
        {
            if (analyzer.isFunctionUsed(funcNode->getName()))
            {
                // This function is used, analyze its body for dependencies
                if (UDE_DEBUG)
                {
                    std::cout << "[UDE] Analyzing body of used function: " << funcNode->getName() << "\n";
                }
                analyzeAST(funcNode->getBodyPtr(), analyzer);
            }
            return;  // Don't recurse further for function nodes
        }

        if (auto* classNode = dynamic_cast<ClassNode*>(node))
        {
            if (analyzer.isClassUsed(classNode->getClassName()))
            {
                // This class is used, analyze its fields and methods for dependencies
                if (UDE_DEBUG)
                {
                    std::cout << "[UDE] Analyzing fields and methods of used class: " << classNode->getClassName() << "\n";
                }

                // Analyze field types - if a class has a field of type X, then X is used
                for (const auto& field : classNode->getFields())
                {
                    // Cast to FieldNode to access field properties
                    if (auto* fieldNode = dynamic_cast<FieldNode*>(field.get()))
                    {
                        // Get the field type - use GenericType API
                        auto fieldType = fieldNode->getGenericType();
                        if (fieldType)
                        {
                            std::string typeName = fieldType->getBaseTypeName();

                            // Check if this is a known class type (not a primitive)
                            if (typeName != "int" && typeName != "float" && typeName != "bool" &&
                                typeName != "string" && typeName != "void" && typeName != "null")
                            {
                                analyzer.analyzeUsedClass(typeName);
                                if (UDE_DEBUG)
                                {
                                    std::cout << "[UDE]   Field '" << fieldNode->getName() << "' uses class: " << typeName << "\n";
                                }
                            }
                        }
                    }
                }

                // Mark methods as used if they're called anywhere in the code
                // (Since we don't have full type inference, we use method name matching)
                for (const auto& method : classNode->getMethods())
                {
                    if (auto* methodNode = dynamic_cast<ast::nodes::classes::MethodNode*>(method.get()))
                    {
                        std::string methodName = methodNode->getName();
                        bool isStatic = methodNode->getIsStatic();
                        std::string className = classNode->getClassName();

                        // Build the correct key based on static/instance
                        std::string methodKey = className + "::" + (isStatic ? "STATIC::" : "") + methodName;

                        // Mark method as used if:
                        // 1. It's already marked as used (special methods, or explicit static calls)
                        // 2. For instance methods: There's a method call with this name in the code
                        bool isUsed = analyzer.usedMethods.find(methodKey) != analyzer.usedMethods.end();
                        if (!isUsed && !isStatic)
                        {
                            // For instance methods, also check by name
                            isUsed = analyzer.calledMethodNames.find(methodName) != analyzer.calledMethodNames.end();
                        }

                        if (isUsed)
                        {
                            analyzer.usedMethods.insert(methodKey);
                            if (UDE_DEBUG)
                            {
                                std::cout << "[UDE]   Marking method as used: " << methodKey << "\n";
                            }
                        }

                        // Always analyze method bodies for dependencies (even if method might be unused)
                        analyzeAST(methodNode->getBodyPtr(), analyzer);
                    }
                }
            }
            return;  // Don't recurse further for class nodes
        }

        // Handle ImportNodes - recurse into imported AST
        if (auto* importNode = dynamic_cast<ImportNode*>(node))
        {
            if (importNode->getImportedAST())
            {
                if (UDE_DEBUG)
                {
                    std::cout << "[UDE] Analyzing used declarations in import: " << importNode->getFilePath() << "\n";
                }
                analyzeUsedDeclarations(importNode->getImportedAST(), analyzer);
            }
            return;
        }

        // For container nodes, recursively process children
        if (auto* programNode = dynamic_cast<ProgramNode*>(node))
        {
            // Process all statements - but the function/class handlers above will check if they're used
            for (const auto& stmt : programNode->getStatements())
            {
                analyzeUsedDeclarations(stmt.get(), analyzer);
            }
            return;
        }

        // For other nodes, don't recurse (we only care about top-level declarations)
    }


    // ================= Dead Declaration Removal (Phase 2) =================

    std::unique_ptr<ast::ASTNode> UnusedDeclarationEliminationPass::removeUnusedDeclarations(
        std::unique_ptr<ast::ASTNode> node,
        const UsageAnalyzer& analyzer)
    {
        if (!node) return nullptr;

        // ProgramNode: Filter top-level declarations
        if (auto* programNode = dynamic_cast<ProgramNode*>(node.get()))
        {
            std::vector<std::unique_ptr<ast::ASTNode>> keptStatements;

            for (const auto& stmt : programNode->getStatements())
            {
                bool keep = true;

                // ImportNodes are always kept (they're just metadata)
                // Note: ImportNodes will be optimized separately in the optimizeImportedAST pass
                if (dynamic_cast<ImportNode*>(stmt.get()))
                {
                    keptStatements.push_back(stmt->clone());
                    continue;
                }

                // Check if this is an unused declaration
                if (auto* funcNode = dynamic_cast<FunctionNode*>(stmt.get()))
                {
                    if (!analyzer.isFunctionUsed(funcNode->getName()))
                    {
                        keep = false;
                        removedFunctions++;
                        if (UDE_DEBUG)
                        {
                            std::cout << "[UDE] Removing unused function: " << funcNode->getName() << "\n";
                        }
                    }
                }
                else if (auto* classNode = dynamic_cast<ClassNode*>(stmt.get()))
                {
                    if (!analyzer.isClassUsed(classNode->getClassName()))
                    {
                        keep = false;
                        removedClasses++;
                        if (UDE_DEBUG)
                        {
                            std::cout << "[UDE] Removing unused class: " << classNode->getClassName() << "\n";
                        }
                    }
                    else
                    {
                        // Class is used, but check if it has unused methods
                        std::string className = classNode->getClassName();
                        bool hasUnusedMethods = false;

                        // Count how many methods are unused
                        for (const auto& method : classNode->getMethods())
                        {
                            if (auto* methodNode = dynamic_cast<MethodNode*>(method.get()))
                            {
                                if (!analyzer.isMethodUsed(className, methodNode->getName()))
                                {
                                    hasUnusedMethods = true;
                                    break;
                                }
                            }
                        }

                        if (hasUnusedMethods)
                        {
                            // Clone class with only used methods
                            auto optimizedClass = classNode->clone();
                            auto* optClassNode = dynamic_cast<ClassNode*>(optimizedClass.get());

                            // Create new class with filtered methods
                            auto newClass = std::make_unique<ClassNode>(
                                optClassNode->getClassName(),
                                optClassNode->getGenericParameters(),
                                optClassNode->getParentClassName(),
                                optClassNode->getImplementedInterfaces(),
                                optClassNode->getLocation()
                            );
                            newClass->setVisibility(optClassNode->getVisibility());
                            newClass->setFinal(optClassNode->isFinal());

                            // Copy all fields
                            for (const auto& field : optClassNode->getFields())
                            {
                                newClass->addField(field->clone());
                            }

                            // Copy all constructors (always needed)
                            for (const auto& ctor : optClassNode->getConstructors())
                            {
                                newClass->addConstructor(ctor->clone());
                            }

                            // Copy only used methods
                            for (const auto& method : optClassNode->getMethods())
                            {
                                if (auto* methodNode = dynamic_cast<MethodNode*>(method.get()))
                                {
                                    if (analyzer.isMethodUsed(className, methodNode->getName()))
                                    {
                                        newClass->addMethod(method->clone());
                                    }
                                    else
                                    {
                                        removedMethods++;
                                        if (UDE_DEBUG)
                                        {
                                            std::cout << "[UDE] Removing unused method: " << className << "::" << methodNode->getName() << "\n";
                                        }
                                    }
                                }
                            }

                            keptStatements.push_back(std::move(newClass));
                            continue;
                        }
                    }
                }
                else if (auto* ifaceNode = dynamic_cast<InterfaceNode*>(stmt.get()))
                {
                    if (!analyzer.isInterfaceUsed(ifaceNode->getName()))
                    {
                        keep = false;
                        removedInterfaces++;
                        if (UDE_DEBUG)
                        {
                            std::cout << "[UDE] Removing unused interface: " << ifaceNode->getName() << "\n";
                        }
                    }
                }
                // Note: We do NOT remove top-level variable declarations/assignments
                // Top-level variables are part of the entry point code and execute immediately
                // They're not like functions/classes which are just declarations that can be unused

                if (keep)
                {
                    // Clone the statement since we're iterating over a const reference
                    keptStatements.push_back(stmt->clone());
                }
            }

            return std::make_unique<ProgramNode>(std::move(keptStatements), programNode->getLocation());
        }

        // For other nodes, just clone
        return node;
    }

    // ================= Imported AST Optimization =================

    void UnusedDeclarationEliminationPass::optimizeImportedAST(
        ast::ASTNode* importedAST,
        const UsageAnalyzer& analyzer,
        std::unordered_set<ast::ASTNode*>& optimizedASTs)
    {
        if (!importedAST) return;

        // Check if we've already optimized this AST (avoid duplicate work)
        if (optimizedASTs.find(importedAST) != optimizedASTs.end())
        {
            if (UDE_DEBUG)
            {
                std::cout << "[UDE] Imported AST already optimized, skipping\n";
            }
            return;
        }

        // Mark as optimized to avoid infinite loops
        optimizedASTs.insert(importedAST);

        if (UDE_DEBUG)
        {
            std::cout << "[UDE] Optimizing imported AST in-place\n";
        }

        // Must be a ProgramNode to optimize
        auto* programNode = dynamic_cast<ProgramNode*>(importedAST);
        if (!programNode)
        {
            if (UDE_DEBUG)
            {
                std::cout << "[UDE] WARNING: Imported AST is not a ProgramNode, cannot optimize\n";
            }
            return;
        }

        // First, recursively optimize any nested imports in this AST
        for (const auto& stmt : programNode->getStatements())
        {
            if (auto* importNode = dynamic_cast<ImportNode*>(stmt.get()))
            {
                if (importNode->getImportedAST())
                {
                    if (UDE_DEBUG)
                    {
                        std::cout << "[UDE] Found nested import: " << importNode->getFilePath() << "\n";
                    }
                    optimizeImportedAST(importNode->getImportedAST(), analyzer, optimizedASTs);
                }
            }
        }

        // Now optimize this ProgramNode's declarations
        std::vector<std::unique_ptr<ast::ASTNode>> keptStatements;

        for (const auto& stmt : programNode->getStatements())
        {
            bool keep = true;

            // ImportNodes are always kept (they're just metadata)
            if (dynamic_cast<ImportNode*>(stmt.get()))
            {
                keptStatements.push_back(stmt->clone());
                continue;
            }

            // Check if this is an unused declaration
            if (auto* funcNode = dynamic_cast<FunctionNode*>(stmt.get()))
            {
                if (!analyzer.isFunctionUsed(funcNode->getName()))
                {
                    keep = false;
                    removedFunctions++;
                    if (UDE_DEBUG)
                    {
                        std::cout << "[UDE] Removing unused function from import: " << funcNode->getName() << "\n";
                    }
                }
            }
            else if (auto* classNode = dynamic_cast<ClassNode*>(stmt.get()))
            {
                if (!analyzer.isClassUsed(classNode->getClassName()))
                {
                    keep = false;
                    removedClasses++;
                    if (UDE_DEBUG)
                    {
                        std::cout << "[UDE] Removing unused class from import: " << classNode->getClassName() << "\n";
                    }
                }
                else
                {
                    // Class is used, but check if it has unused methods
                    std::string className = classNode->getClassName();
                    bool hasUnusedMethods = false;

                    // Count how many methods are unused
                    for (const auto& method : classNode->getMethods())
                    {
                        if (auto* methodNode = dynamic_cast<MethodNode*>(method.get()))
                        {
                            if (!analyzer.isMethodUsed(className, methodNode->getName()))
                            {
                                hasUnusedMethods = true;
                                break;
                            }
                        }
                    }

                    if (hasUnusedMethods)
                    {
                        // Clone class with only used methods
                        auto optimizedClass = classNode->clone();
                        auto* optClassNode = dynamic_cast<ClassNode*>(optimizedClass.get());

                        // Create new class with filtered methods
                        auto newClass = std::make_unique<ClassNode>(
                            optClassNode->getClassName(),
                            optClassNode->getGenericParameters(),
                            optClassNode->getParentClassName(),
                            optClassNode->getImplementedInterfaces(),
                            optClassNode->getLocation()
                        );
                        newClass->setVisibility(optClassNode->getVisibility());
                        newClass->setFinal(optClassNode->isFinal());

                        // Copy all fields
                        for (const auto& field : optClassNode->getFields())
                        {
                            newClass->addField(field->clone());
                        }

                        // Copy all constructors (always needed)
                        for (const auto& ctor : optClassNode->getConstructors())
                        {
                            newClass->addConstructor(ctor->clone());
                        }

                        // Copy only used methods
                        for (const auto& method : optClassNode->getMethods())
                        {
                            if (auto* methodNode = dynamic_cast<MethodNode*>(method.get()))
                            {
                                if (analyzer.isMethodUsed(className, methodNode->getName()))
                                {
                                    newClass->addMethod(method->clone());
                                }
                                else
                                {
                                    removedMethods++;
                                    if (UDE_DEBUG)
                                    {
                                        std::cout << "[UDE] Removing unused method from import: " << className << "::" << methodNode->getName() << "\n";
                                    }
                                }
                            }
                        }

                        keptStatements.push_back(std::move(newClass));
                        continue;
                    }
                }
            }
            else if (auto* ifaceNode = dynamic_cast<InterfaceNode*>(stmt.get()))
            {
                if (!analyzer.isInterfaceUsed(ifaceNode->getName()))
                {
                    keep = false;
                    removedInterfaces++;
                    if (UDE_DEBUG)
                    {
                        std::cout << "[UDE] Removing unused interface from import: " << ifaceNode->getName() << "\n";
                    }
                }
            }

            if (keep)
            {
                keptStatements.push_back(stmt->clone());
            }
        }

        // CRITICAL: Modify the ProgramNode in-place by replacing its statements
        // We need to use const_cast because we have a non-const pointer to the cached AST
        // This is safe because we own the cache through ImportManager
        programNode->setStatements(std::move(keptStatements));

        if (UDE_DEBUG)
        {
            std::cout << "[UDE] Imported AST optimized successfully\n";
        }
    }

    // ================= UnusedDeclarationEliminationPass Implementation =================

    UnusedDeclarationEliminationPass::UnusedDeclarationEliminationPass()
        : OptimizationPass("UnusedDeclarationElimination", base::PassType::TRANSFORMATION)
    {
    }

    std::unique_ptr<ast::ASTNode> UnusedDeclarationEliminationPass::optimize(
        std::unique_ptr<ast::ASTNode> node,
        base::OptimizationContext& context)
    {
        if (UDE_DEBUG)
        {
            std::cout << "[UDE] ===== Starting Unused Declaration Elimination Pass =====\n";
        }

        auto startTime = std::chrono::high_resolution_clock::now();

        // Phase 1a: Analyze declarations and direct usages from entry point
        UsageAnalyzer analyzer;
        analyzeAST(node.get(), analyzer);

        if (UDE_DEBUG)
        {
            std::cout << "[UDE] After Phase 1a - Used functions: " << analyzer.usedFunctions.size() << "\n";
            for (const auto& func : analyzer.usedFunctions)
            {
                std::cout << "[UDE]   - " << func << "\n";
            }
            std::cout << "[UDE] After Phase 1a - Used classes: " << analyzer.usedClasses.size() << "\n";
            for (const auto& cls : analyzer.usedClasses)
            {
                std::cout << "[UDE]   - " << cls << "\n";
            }
        }

        // Phase 1b: Recursively analyze bodies of used declarations until no new dependencies found
        if (UDE_DEBUG)
        {
            std::cout << "[UDE] Phase 1b: Analyzing used declarations recursively...\n";
        }

        bool changed = true;
        int iteration = 0;
        while (changed)
        {
            iteration++;
            size_t beforeFunctions = analyzer.usedFunctions.size();
            size_t beforeClasses = analyzer.usedClasses.size();

            if (UDE_DEBUG)
            {
                std::cout << "[UDE] Iteration " << iteration << "...\n";
            }

            analyzeUsedDeclarations(node.get(), analyzer);

            size_t afterFunctions = analyzer.usedFunctions.size();
            size_t afterClasses = analyzer.usedClasses.size();

            changed = (afterFunctions != beforeFunctions) || (afterClasses != beforeClasses);

            if (UDE_DEBUG && changed)
            {
                std::cout << "[UDE] Found new dependencies: "
                          << (afterFunctions - beforeFunctions) << " functions, "
                          << (afterClasses - beforeClasses) << " classes\n";
            }
        }

        // Compute transitive closure
        analyzer.computeTransitiveClosure();

        if (UDE_DEBUG)
        {
            std::cout << "[UDE] Declared functions: " << analyzer.declaredFunctions.size() << "\n";
            for (const auto& func : analyzer.declaredFunctions)
            {
                std::cout << "[UDE]   - " << func << "\n";
            }
            std::cout << "[UDE] Used functions: " << analyzer.usedFunctions.size() << "\n";
            for (const auto& func : analyzer.usedFunctions)
            {
                std::cout << "[UDE]   - " << func << "\n";
            }
            std::cout << "[UDE] Declared classes: " << analyzer.declaredClasses.size() << "\n";
            for (const auto& cls : analyzer.declaredClasses)
            {
                std::cout << "[UDE]   - " << cls << "\n";
            }
            std::cout << "[UDE] Used classes: " << analyzer.usedClasses.size() << "\n";
            for (const auto& cls : analyzer.usedClasses)
            {
                std::cout << "[UDE]   - " << cls << "\n";
            }
            std::cout << "[UDE] Called method names: " << analyzer.calledMethodNames.size() << "\n";
            for (const auto& method : analyzer.calledMethodNames)
            {
                std::cout << "[UDE]   - " << method << "()\n";
            }
            std::cout << "[UDE] Declared methods: " << analyzer.declaredMethods.size() << "\n";
            for (const auto& method : analyzer.declaredMethods)
            {
                std::cout << "[UDE]   - " << method << "\n";
            }
            std::cout << "[UDE] Used methods: " << analyzer.usedMethods.size() << "\n";
            for (const auto& method : analyzer.usedMethods)
            {
                std::cout << "[UDE]   - " << method << "\n";
            }
        }

        // Phase 2a: Optimize imported ASTs first (in-place modification)
        if (UDE_DEBUG)
        {
            std::cout << "[UDE] Phase 2a: Optimizing imported ASTs...\n";
        }
        std::unordered_set<ast::ASTNode*> optimizedASTs;
        if (auto* programNode = dynamic_cast<ProgramNode*>(node.get()))
        {
            for (const auto& stmt : programNode->getStatements())
            {
                if (auto* importNode = dynamic_cast<ImportNode*>(stmt.get()))
                {
                    if (importNode->getImportedAST())
                    {
                        if (UDE_DEBUG)
                        {
                            std::cout << "[UDE] Optimizing import: " << importNode->getFilePath() << "\n";
                        }
                        optimizeImportedAST(importNode->getImportedAST(), analyzer, optimizedASTs);
                    }
                }
            }
        }

        // Phase 2b: Remove unused declarations from main AST
        if (UDE_DEBUG)
        {
            std::cout << "[UDE] Phase 2b: Removing unused declarations from main AST...\n";
        }
        auto result = removeUnusedDeclarations(std::move(node), analyzer);

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        setExecutionTime(duration);

        size_t totalRemoved = removedFunctions + removedClasses + removedInterfaces + removedVariables + removedMethods;

        if (totalRemoved > 0)
        {
            context.setModified(true);
            context.recordTransformation("UnusedDeclarationElimination");
        }

        if (UDE_DEBUG)
        {
            std::cout << "\n[UDE] ===== OPTIMIZATION SUMMARY =====\n";
            std::cout << "[UDE] Removed " << removedFunctions << " unused functions\n";
            std::cout << "[UDE] Removed " << removedClasses << " unused classes\n";
            std::cout << "[UDE] Removed " << removedInterfaces << " unused interfaces\n";
            std::cout << "[UDE] Removed " << removedVariables << " unused variables\n";
            std::cout << "[UDE] Removed " << removedMethods << " unused methods\n";
            std::cout << "[UDE] Total declarations removed: " << totalRemoved << "\n";
            std::cout << "[UDE] ===== Unused Declaration Elimination Pass Complete =====\n\n";
        }

        return result;
    }

    std::string UnusedDeclarationEliminationPass::getDescription() const
    {
        return "Removes unused functions, classes, interfaces, and variables (preserves public/entry point)";
    }

    void UnusedDeclarationEliminationPass::reportMetrics(OptimizationResult& result) const
    {
        size_t totalRemoved = removedFunctions + removedClasses + removedInterfaces + removedVariables + removedMethods;

        optimizer::PassMetrics metrics(
            getName(),
            totalRemoved,
            getExecutionTime(),
            totalRemoved > 0
        );
        result.addPassMetrics(metrics);
    }

    void UnusedDeclarationEliminationPass::reset()
    {
        OptimizationPass::reset();
        removedFunctions = 0;
        removedClasses = 0;
        removedInterfaces = 0;
        removedVariables = 0;
        removedMethods = 0;
    }
} // namespace optimizer::passes
