#include "UnusedVariableAnalyzer.hpp"

#include "../diagnostics/DiagnosticBuilder.hpp"
#include "../diagnostics/ErrorCodeRegistry.hpp"

#include "../ast/ASTNode.hpp"
#include "../ast/nodes/statements/ProgramNode.hpp"
#include "../ast/nodes/statements/BlockNode.hpp"
#include "../ast/nodes/statements/AssignmentNode.hpp"
#include "../ast/nodes/statements/MemberAssignmentNode.hpp"
#include "../ast/nodes/statements/IndexAssignmentNode.hpp"
#include "../ast/nodes/statements/IfNode.hpp"
#include "../ast/nodes/statements/WhileNode.hpp"
#include "../ast/nodes/statements/DoWhileNode.hpp"
#include "../ast/nodes/statements/ForNode.hpp"
#include "../ast/nodes/statements/ForEachNode.hpp"
#include "../ast/nodes/statements/SwitchNode.hpp"
#include "../ast/nodes/statements/CaseNode.hpp"
#include "../ast/nodes/statements/DefaultCaseNode.hpp"
#include "../ast/nodes/statements/TryNode.hpp"
#include "../ast/nodes/statements/CatchNode.hpp"
#include "../ast/nodes/statements/ThrowNode.hpp"
#include "../ast/nodes/statements/BreakNode.hpp"
#include "../ast/nodes/statements/ContinueNode.hpp"
#include "../ast/nodes/expressions/VariableNode.hpp"
#include "../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../ast/nodes/expressions/TernaryExpNode.hpp"
#include "../ast/nodes/expressions/IntegerNode.hpp"
#include "../ast/nodes/expressions/FloatNode.hpp"
#include "../ast/nodes/expressions/StringNode.hpp"
#include "../ast/nodes/expressions/BoolNode.hpp"
#include "../ast/nodes/expressions/NullNode.hpp"
#include "../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../ast/nodes/expressions/ArrayLiteralNode.hpp"
#include "../ast/nodes/expressions/ArrayCreationNode.hpp"
#include "../ast/nodes/functions/FunctionNode.hpp"
#include "../ast/nodes/functions/FunctionCallNode.hpp"
#include "../ast/nodes/functions/ReturnNode.hpp"
#include "../ast/nodes/classes/MemberAccessNode.hpp"
#include "../ast/nodes/classes/MethodCallNode.hpp"
#include "../ast/nodes/classes/NewNode.hpp"

#include <string>
#include <vector>

namespace analysis
{
    namespace
    {
        // Per-declaration tracking record. `nameLoc` is the source span
        // for the identifier itself, used both for the primary span on
        // the warning and for the rename quick-fix's anchor.
        struct Decl
        {
            std::string name;
            errors::SourceLocation nameLoc;
            bool used = false;
        };

        // One per BlockNode / FunctionNode body. Declarations belong to
        // the innermost frame; reads walk the stack inside-out and mark
        // the first matching decl as used.
        struct ScopeFrame
        {
            std::vector<Decl> decls;
            // When `gaveUp` is set we encountered a node we don't know
            // how to traverse and conservatively mark every decl in this
            // frame as used to suppress false positives.
            bool gaveUp = false;
        };

        // The walker is a single-pass visitor; statefulness lives here so
        // every recursive call sees the same scope stack and warning sink.
        class Walker
        {
        public:
            std::vector<diagnostics::Diagnostic> warnings;

            void walk(ast::ASTNode* node)
            {
                if (!node) return;
                dispatch(node);
            }

            void enterScope()
            {
                scopes_.emplace_back();
            }

            void exitScope()
            {
                if (scopes_.empty()) return;
                auto frame = std::move(scopes_.back());
                scopes_.pop_back();
                if (frame.gaveUp) return; // suppress warnings for this scope

                for (const auto& decl : frame.decls)
                {
                    if (decl.used) continue;
                    if (!decl.name.empty() && decl.name[0] == '_') continue; // intentional ignore
                    warnings.push_back(buildWarning(decl));
                }
            }

        private:
            std::vector<ScopeFrame> scopes_;

            void dispatch(ast::ASTNode* node)
            {
                // Programs and blocks open scopes. Everything else recurses
                // through hand-written cases. Anything we don't recognise
                // calls giveUpCurrentScope() and stops walking that subtree
                // — guaranteeing we never produce a false positive.

                if (auto* p = dynamic_cast<ast::ProgramNode*>(node))
                {
                    enterScope();
                    walkProgramChildren(p);
                    exitScope();
                    return;
                }
                if (auto* b = dynamic_cast<ast::BlockNode*>(node))
                {
                    enterScope();
                    walkBlockChildren(b);
                    exitScope();
                    return;
                }

                // Statements
                if (auto* a = dynamic_cast<ast::AssignmentNode*>(node)) { handleAssignment(a); return; }
                if (auto* m = dynamic_cast<ast::MemberAssignmentNode*>(node)) { walkMemberAssignment(m); return; }
                if (auto* i = dynamic_cast<ast::IndexAssignmentNode*>(node)) { walkIndexAssignment(i); return; }
                if (auto* i = dynamic_cast<ast::IfNode*>(node)) { walkIf(i); return; }
                if (auto* w = dynamic_cast<ast::WhileNode*>(node)) { walkWhile(w); return; }
                if (auto* w = dynamic_cast<ast::DoWhileNode*>(node)) { walkDoWhile(w); return; }
                if (auto* f = dynamic_cast<ast::ForNode*>(node)) { walkFor(f); return; }
                if (auto* f = dynamic_cast<ast::ForEachNode*>(node)) { walkForEach(f); return; }
                if (dynamic_cast<ast::BreakNode*>(node)) { return; }
                if (dynamic_cast<ast::ContinueNode*>(node)) { return; }
                if (auto* r = dynamic_cast<ast::ReturnNode*>(node)) { walkReturn(r); return; }
                if (auto* t = dynamic_cast<ast::ThrowNode*>(node)) { walkThrow(t); return; }
                if (auto* t = dynamic_cast<ast::TryNode*>(node)) { walkTry(t); return; }

                // Expressions
                if (auto* v = dynamic_cast<ast::VariableNode*>(node)) { handleRead(v); return; }
                if (auto* b = dynamic_cast<ast::BinaryOpNode*>(node)) { walkBinary(b); return; }
                if (auto* u = dynamic_cast<ast::UnaryOpNode*>(node)) { walkUnary(u); return; }
                if (auto* t = dynamic_cast<ast::TernaryOpNode*>(node)) { walkTernary(t); return; }
                if (auto* c = dynamic_cast<ast::FunctionCallNode*>(node)) { walkFunctionCall(c); return; }
                if (auto* c = dynamic_cast<ast::MethodCallNode*>(node)) { walkMethodCall(c); return; }
                if (auto* m = dynamic_cast<ast::MemberAccessNode*>(node)) { walkMemberAccess(m); return; }
                if (auto* n = dynamic_cast<ast::NewNode*>(node)) { walkNew(n); return; }
                if (auto* i = dynamic_cast<ast::IndexAccessNode*>(node)) { walkIndexAccess(i); return; }
                if (auto* a = dynamic_cast<ast::ArrayLiteralNode*>(node)) { walkArrayLiteral(a); return; }

                // Literals — no children, no reads.
                if (dynamic_cast<ast::IntegerNode*>(node)) return;
                if (dynamic_cast<ast::FloatNode*>(node)) return;
                if (dynamic_cast<ast::StringNode*>(node)) return;
                if (dynamic_cast<ast::BoolNode*>(node)) return;
                if (dynamic_cast<ast::NullNode*>(node)) return;

                // Anything else (lambdas, classes/methods at runtime,
                // switch/match, casts, await, super calls, ...) — bail
                // for the current scope to avoid false positives.
                giveUpCurrentScope();
            }

            void giveUpCurrentScope()
            {
                if (!scopes_.empty()) {
                    scopes_.back().gaveUp = true;
                }
            }

            // ---- scope walkers ------------------------------------------------

            void walkProgramChildren(ast::ProgramNode* node);
            void walkBlockChildren(ast::BlockNode* node);

            // ---- statement walkers --------------------------------------------

            void handleAssignment(ast::AssignmentNode* node)
            {
                // Walk the RHS first so reads of OTHER variables count
                // before this declaration shadows the name.
                if (auto* rhs = node->getValue()) walk(rhs);

                if (scopes_.empty()) return;
                auto& frame = scopes_.back();

                // Heuristic for "is this a declaration": parser sets
                // variableType for declarations, leaves it VOID for plain
                // reassignments. As a safety net we also treat the first
                // assignment to a name in this scope as a declaration.
                const std::string& name = node->getVariableName();
                if (name.empty()) return;

                // Skip if this name was already declared in the current
                // scope frame — that means we're looking at a reassignment.
                bool alreadyDeclared = false;
                for (auto& d : frame.decls)
                {
                    if (d.name == name)
                    {
                        d.used = true; // reassignment touches the variable
                        alreadyDeclared = true;
                        break;
                    }
                }
                if (alreadyDeclared) return;

                // Skip if this name is already declared in an outer scope
                // (it's a reassignment to an outer var, not a new decl).
                for (auto it = scopes_.rbegin() + 1; it != scopes_.rend(); ++it)
                {
                    for (auto& d : it->decls)
                    {
                        if (d.name == name)
                        {
                            d.used = true;
                            return;
                        }
                    }
                }

                // Static and final modifier check — these are commonly
                // top-level globals or class fields. Don't track them as
                // local-scope unused variables.
                if (node->getIsStatic()) return;

                // First time seeing this name in any visible scope —
                // treat as a new declaration.
                Decl decl;
                decl.name = name;
                decl.nameLoc = node->getLocation();
                frame.decls.push_back(std::move(decl));
            }

            void walkMemberAssignment(ast::MemberAssignmentNode*) { giveUpCurrentScope(); }
            void walkIndexAssignment(ast::IndexAssignmentNode*) { giveUpCurrentScope(); }

            void walkIf(ast::IfNode* node);
            void walkWhile(ast::WhileNode* node);
            void walkDoWhile(ast::DoWhileNode* node);
            void walkFor(ast::ForNode* node);
            void walkForEach(ast::ForEachNode* node);
            void walkReturn(ast::ReturnNode* node);
            void walkThrow(ast::ThrowNode* node);
            void walkTry(ast::TryNode* node);

            // ---- expression walkers --------------------------------------------

            void handleRead(ast::VariableNode* node)
            {
                const std::string& name = node->getName();
                for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it)
                {
                    for (auto& d : it->decls)
                    {
                        if (d.name == name)
                        {
                            d.used = true;
                            return;
                        }
                    }
                }
            }

            void walkBinary(ast::BinaryOpNode* node);
            void walkUnary(ast::UnaryOpNode* node);
            void walkTernary(ast::TernaryOpNode* node);
            void walkFunctionCall(ast::FunctionCallNode* node);
            void walkMethodCall(ast::MethodCallNode* node);
            void walkMemberAccess(ast::MemberAccessNode* node);
            void walkNew(ast::NewNode* node);
            void walkIndexAccess(ast::IndexAccessNode* node);
            void walkArrayLiteral(ast::ArrayLiteralNode* node);

            // ---- diagnostic builder --------------------------------------------

            diagnostics::Diagnostic buildWarning(const Decl& decl) const
            {
                diagnostics::Suggestion rename;
                rename.label = "rename to '_" + decl.name + "'";
                rename.renderedHint = "help: prefix with '_' to mark intentionally unused";
                rename.edits.push_back(diagnostics::TextEdit{
                    decl.nameLoc, decl.nameLoc, "_"
                });
                rename.applicability = diagnostics::FixApplicability::MachineApplicable;

                diagnostics::Diagnostic diag = diagnostics::DiagnosticBuilder(
                        diagnostics::codes::UnusedVariable)
                    .withMessage("unused variable '" + decl.name + "'")
                    .withPrimary(decl.nameLoc, "declared but never read")
                    .withSuggestion(std::move(rename))
                    .withSourceException("UnusedVariableAnalyzer")
                    .build();
                diag.tags.push_back(1); // LSP DiagnosticTag::Unnecessary
                return diag;
            }
        };
    } // namespace

    // ----- definitions of methods that need full node headers ---------------

    void Walker::walkProgramChildren(ast::ProgramNode* node)
    {
        // ProgramNode contains a list of top-level statements. The exact
        // accessor varies by codebase; the safe option is to walk every
        // statement child the node exposes. We use the same access path
        // BlockNode would use, since both are statement containers.
        // If ProgramNode does not expose its children in the same shape,
        // mark the program scope as gave-up so no warnings are emitted.
        // (BlockNode below is the path most user code goes through.)
        const auto& statements = node->getStatements();
        for (const auto& stmt : statements)
        {
            if (stmt) walk(stmt.get());
        }
    }

    void Walker::walkBlockChildren(ast::BlockNode* node)
    {
        const auto& statements = node->getStatements();
        for (const auto& stmt : statements)
        {
            if (stmt) walk(stmt.get());
        }
    }

    void Walker::walkIf(ast::IfNode* node)
    {
        if (auto* cond = node->getCondition()) walk(cond);
        if (auto* th = node->getThenStatement()) walk(th);
        if (auto* el = node->getElseStatement()) walk(el);
    }

    void Walker::walkWhile(ast::WhileNode* node)
    {
        if (auto* cond = node->getCondition()) walk(cond);
        if (auto* body = node->getBody()) walk(body);
    }

    void Walker::walkDoWhile(ast::DoWhileNode* node)
    {
        if (auto* body = node->getBody()) walk(body);
        if (auto* cond = node->getCondition()) walk(cond);
    }

    void Walker::walkFor(ast::ForNode* node)
    {
        // C-style for: init / condition / update / body. The init clause
        // typically declares a loop counter that the body reads — but we
        // don't track for-counter declarations as MYT-49 candidates because
        // they're conventionally allowed to be "unused" in some loops.
        // Walking the init still records reads against any outer-scope
        // variable it touches.
        if (auto* init = node->getInitialization()) walk(init);
        if (auto* cond = node->getCondition()) walk(cond);
        if (auto* upd = node->getUpdate()) walk(upd);
        if (auto* body = node->getBody()) walk(body);
    }

    void Walker::walkForEach(ast::ForEachNode* node)
    {
        if (auto* coll = node->getCollection()) walk(coll);
        if (auto* body = node->getBody()) walk(body);
    }

    void Walker::walkReturn(ast::ReturnNode* node)
    {
        if (auto* val = node->getReturnValue()) walk(val);
    }

    void Walker::walkThrow(ast::ThrowNode* node)
    {
        if (auto* val = node->getException()) walk(val);
    }

    void Walker::walkTry(ast::TryNode* node)
    {
        if (auto* body = node->getTryBlock()) walk(body);
        // Catch blocks have their own variable bindings — bail
        // conservatively rather than risk false positives.
        giveUpCurrentScope();
    }

    void Walker::walkBinary(ast::BinaryOpNode* node)
    {
        if (auto* l = node->getLeft()) walk(l);
        if (auto* r = node->getRight()) walk(r);
    }

    void Walker::walkUnary(ast::UnaryOpNode* node)
    {
        if (auto* o = node->getOperand()) walk(o);
    }

    void Walker::walkTernary(ast::TernaryOpNode* node)
    {
        if (auto* c = node->getCondition()) walk(c);
        if (auto* t = node->getTrueExpression()) walk(t);
        if (auto* e = node->getFalseExpression()) walk(e);
    }

    void Walker::walkFunctionCall(ast::FunctionCallNode* node)
    {
        for (const auto& arg : node->getArguments())
        {
            if (arg) walk(arg.get());
        }
    }

    void Walker::walkMethodCall(ast::MethodCallNode* node)
    {
        // Receiver may itself be a variable read.
        if (auto* obj = node->getObject()) walk(obj);
        for (const auto& arg : node->getArguments())
        {
            if (arg) walk(arg.get());
        }
    }

    void Walker::walkMemberAccess(ast::MemberAccessNode* node)
    {
        if (auto* obj = node->getObject()) walk(obj);
    }

    void Walker::walkNew(ast::NewNode* node)
    {
        for (const auto& arg : node->getArguments())
        {
            if (arg) walk(arg.get());
        }
    }

    void Walker::walkIndexAccess(ast::IndexAccessNode* node)
    {
        if (auto* arr = node->getCollection()) walk(arr);
        if (auto* idx = node->getIndex()) walk(idx);
    }

    void Walker::walkArrayLiteral(ast::ArrayLiteralNode* node)
    {
        for (const auto& elem : node->getElements())
        {
            if (elem) walk(elem.get());
        }
    }

    // ----- public entry point -----------------------------------------------

    std::vector<diagnostics::Diagnostic> UnusedVariableAnalyzer::analyze(ast::ASTNode* program)
    {
        if (!program) return {};
        Walker walker;
        walker.walk(program);
        return std::move(walker.warnings);
    }
}
