#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace ast
{
    class ASTNode;
    namespace nodes
    {
        namespace statements { class AssignmentNode; }
        namespace classes { class NewNode; }
    }
}

namespace optimizer::passes::detail
{
    // Per-function escape analysis state. Scoped to a single body
    // (FunctionNode / MethodNode / ConstructorNode / LambdaNode / top-level ProgramNode)
    // so nested functions are analyzed independently.
    //
    // Implementation is split across EscapeAnalysisPass.cpp (driver, Phase 1
    // candidate collection, Phase 3 finalize, union-find) and
    // EscapeAnalysisPass_Detection.cpp (Phase 2 walkStmt/walkAssignment/walkExpr,
    // lambda-capture walker).
    class FunctionEscapeAnalyzer
    {
    public:
        explicit FunctionEscapeAnalyzer(std::size_t& promotionsCounter)
            : promotions(promotionsCounter) {}

        void analyze(ast::ASTNode* body)
        {
            if (!body) return;
            collectCandidates(body);
            if (candidates.empty()) return;
            walkStmt(body);
            finalize();
        }

    private:
        enum class Ctx { SAFE, ESCAPING };

        // Candidate: local name → NewNode that initialized it.
        std::unordered_map<std::string, ast::nodes::classes::NewNode*> candidates;
        // Union-find parent map over local names.
        std::unordered_map<std::string, std::string> ufParent;
        // Roots known to escape.
        std::unordered_set<std::string> escapedRoots;

        std::size_t& promotions;

        // Union-find helpers.
        void ensureTracked(const std::string& name);
        std::string findRoot(const std::string& name);
        void unionLocals(const std::string& a, const std::string& b);
        void markEscaped(const std::string& name);

        // Phase 1: collect candidate locals (defined in EscapeAnalysisPass.cpp).
        void collectCandidates(ast::ASTNode* node);
        void recurseForCollection(ast::ASTNode* node);

        // Phase 2: walk for escapes (defined in EscapeAnalysisPass_Detection.cpp).
        void walkStmt(ast::ASTNode* node);
        void walkAssignment(ast::nodes::statements::AssignmentNode* assign);
        void walkExpr(ast::ASTNode* node, Ctx ctx);
        void walkLambdaCaptures(ast::ASTNode* node);

        // Phase 3: commit flags (defined in EscapeAnalysisPass.cpp).
        void finalize();
    };
}
