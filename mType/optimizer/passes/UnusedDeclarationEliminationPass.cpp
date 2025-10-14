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
#include "../../ast/nodes/expressions/VariableNode.hpp"
#include "../../ast/nodes/functions/ReturnNode.hpp"
#include "../../ast/nodes/statements/SwitchNode.hpp"
#include "../../ast/nodes/statements/CaseNode.hpp"
#include <chrono>
#include <iostream>

namespace optimizer::passes
{
    using namespace ast;
    using namespace ast::nodes::statements;
    using namespace ast::nodes::functions;
    using namespace ast::nodes::classes;
    using namespace ast::nodes::expressions;

    // Debug flag
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
            analyzer.analyzeDeclaredClass(classNode->getClassName(), isPublic);

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

            // Don't analyze methods here - we'll do it in second pass for used classes only
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
            for (const auto& stmt : programNode->getStatements())
            {
                analyzeAST(stmt.get(), analyzer);
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

        // For any other node types, we don't need to analyze them for this pass
        // (literals, operators, etc. don't declare or use functions/classes)
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

                // Analyze method bodies
                for (const auto& method : classNode->getMethods())
                {
                    if (auto* methodNode = dynamic_cast<ast::nodes::classes::MethodNode*>(method.get()))
                    {
                        analyzeAST(methodNode->getBodyPtr(), analyzer);
                    }
                }
            }
            return;  // Don't recurse further for class nodes
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
        }

        // Phase 2: Remove unused declarations
        auto result = removeUnusedDeclarations(std::move(node), analyzer);

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        setExecutionTime(duration);

        size_t totalRemoved = removedFunctions + removedClasses + removedInterfaces + removedVariables;

        if (totalRemoved > 0)
        {
            context.setModified(true);
            context.recordTransformation("UnusedDeclarationElimination");
        }

        if (UDE_DEBUG)
        {
            std::cout << "[UDE] Removed: " << removedFunctions << " functions, "
                << removedClasses << " classes, "
                << removedInterfaces << " interfaces, "
                << removedVariables << " variables\n";
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
        size_t totalRemoved = removedFunctions + removedClasses + removedInterfaces + removedVariables;

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
    }
} // namespace optimizer::passes
