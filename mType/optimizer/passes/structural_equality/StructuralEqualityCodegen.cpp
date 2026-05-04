#include "StructuralEqualityCodegen.hpp"
#include <cstdint>

#include "../../../ast/GenericType.hpp"
#include "../../../ast/nodes/classes/ClassNode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../ast/nodes/classes/MemberAccessNode.hpp"
#include "../../../ast/nodes/classes/MethodCallNode.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../../ast/nodes/classes/SuperMethodCallNode.hpp"
#include "../../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../../ast/nodes/expressions/BoolNode.hpp"
#include "../../../ast/nodes/expressions/CastExpression.hpp"
#include "../../../ast/nodes/expressions/InstanceOfExpression.hpp"
#include "../../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../../../ast/nodes/expressions/VariableNode.hpp"
#include "../../../ast/nodes/functions/ReturnNode.hpp"
#include "../../../ast/nodes/statements/AssignmentNode.hpp"
#include "../../../ast/nodes/statements/BlockNode.hpp"
#include "../../../ast/nodes/statements/IfNode.hpp"
#include "../../../token/TokenType.hpp"
#include "../../../value/ValueType.hpp"

namespace optimizer::passes::structural_equality
{
    using namespace ast;
    using namespace ast::nodes::expressions;
    using namespace ast::nodes::statements;
    using namespace ast::nodes::classes;
    using namespace ast::nodes::functions;
    using token::TokenType;
    using value::ValueType;

    namespace
    {
        // ---- Type classification helpers --------------------------------

        // True iff the field is a primitive that supports direct == and direct
        // hash component (no .hashCode() call needed).
        bool isPrimitiveValueType(ValueType vt)
        {
            return vt == ValueType::INT || vt == ValueType::FLOAT || vt == ValueType::BOOL;
        }

        // Resolves a field's GenericType into a coarse "kind" for codegen.
        enum class FieldKind { INT, FLOAT, BOOL, OBJECT };

        FieldKind classifyField(const FieldNode* field)
        {
            auto genericType = field->getGenericType();
            if (!genericType) return FieldKind::OBJECT;
            if (genericType->isGenericParameter())
            {
                // Either a type parameter (handled by Phase 1.5) or a class
                // name. Treat as object — `.equals()`/`.hashCode()` dispatch
                // through normal method resolution.
                return FieldKind::OBJECT;
            }
            ValueType vt = genericType->getConcreteType();
            if (vt == ValueType::INT) return FieldKind::INT;
            if (vt == ValueType::FLOAT) return FieldKind::FLOAT;
            if (vt == ValueType::BOOL) return FieldKind::BOOL;
            return FieldKind::OBJECT;
        }

        bool isFieldNullable(const FieldNode* field)
        {
            auto genericType = field->getGenericType();
            return genericType && genericType->isNullable();
        }

        // ---- Tiny AST builders ------------------------------------------

        std::unique_ptr<ASTNode> makeThisField(const std::string& fieldName)
        {
            std::unique_ptr<ASTNode> receiver = std::make_unique<VariableNode>("this");
            return std::make_unique<MemberAccessNode>(
                std::move(receiver), fieldName, /*isStatic*/ false);
        }

        std::unique_ptr<ASTNode> makeOtherField(const std::string& localName,
                                                const std::string& fieldName)
        {
            std::unique_ptr<ASTNode> receiver = std::make_unique<VariableNode>(localName);
            return std::make_unique<MemberAccessNode>(
                std::move(receiver), fieldName, /*isStatic*/ false);
        }

        std::unique_ptr<ASTNode> makeIntLiteral(int64_t v)
        {
            return std::make_unique<IntegerNode>(v);
        }

        std::unique_ptr<ASTNode> makeBinary(std::unique_ptr<ASTNode> lhs,
                                            TokenType op,
                                            std::unique_ptr<ASTNode> rhs)
        {
            return std::make_unique<BinaryExpNode>(std::move(lhs), op, std::move(rhs));
        }

        std::unique_ptr<ASTNode> makeMethodCall(std::unique_ptr<ASTNode> receiver,
                                                const std::string& methodName)
        {
            std::vector<std::unique_ptr<ASTNode>> args;
            return std::make_unique<MethodCallNode>(
                std::move(receiver), methodName, std::move(args), /*isStatic*/ false);
        }

        std::unique_ptr<ASTNode> makeMethodCall1Arg(std::unique_ptr<ASTNode> receiver,
                                                    const std::string& methodName,
                                                    std::unique_ptr<ASTNode> arg)
        {
            std::vector<std::unique_ptr<ASTNode>> args;
            args.push_back(std::move(arg));
            return std::make_unique<MethodCallNode>(
                std::move(receiver), methodName, std::move(args), /*isStatic*/ false);
        }

        // ---- hashCode component synthesis -------------------------------

        // Given a field, returns the AST that produces its hash int.
        // Caller composes it as `result = 31 * result + <component>`.
        // NOTE: BOOL/FLOAT branches below are unreachable in Phase 1 —
        // StructuralEqualityPolicy::allFieldsSafeForSynthesis gates non-INT
        // primitives out today. Pre-staged so codegen is ready when the
        // gate relaxes in Phase 1.5+.
        std::unique_ptr<ASTNode> makeHashComponent(const FieldNode* field)
        {
            FieldKind kind = classifyField(field);
            std::unique_ptr<ASTNode> thisField = makeThisField(field->getName());

            if (kind == FieldKind::INT)
            {
                return thisField;
            }
            if (kind == FieldKind::BOOL)
            {
                // Phase 1.5+: ternary unavailable cleanly here without
                // complicating the op budget; emit `(this.f ? 1231 : 1237)`
                // via inline expansion would need TernaryExpNode + extra
                // ops. Cheaper: call .hashCode() through Bool wrapper.
                // The protocol fast path handles Bool.hashCode in the JIT.
                return makeMethodCall(std::move(thisField), "hashCode");
            }
            if (kind == FieldKind::FLOAT)
            {
                // Phase 1.5+: same reasoning as BOOL — Float wrapper
                // protocol fast path.
                return makeMethodCall(std::move(thisField), "hashCode");
            }
            // OBJECT: needs null-guard if nullable.
            if (isFieldNullable(field))
            {
                // Lower as an if-statement at the *statement* level; the
                // expression-level hash component returns 0 here, and the
                // synthesizer wraps the statement to handle the call.
                // But for simplicity: emit (this.f == null ? 0 :
                // this.f.hashCode()) using a comparison + method call by
                // having the caller generate the if/else assignment.
                // We signal nullability by returning the .hashCode() call
                // and let the equals-style early-out handle it in the if
                // branch in the statement composer.
                return makeMethodCall(makeThisField(field->getName()), "hashCode");
            }
            return makeMethodCall(std::move(thisField), "hashCode");
        }

        // For a nullable-object field, synthesize:
        //   if (this.f == null) { h = 31 * h + 0; } else { h = 31 * h + this.f.hashCode(); }
        // and append both AssignmentNodes to the statements list. Returns
        // the if-statement.
        std::unique_ptr<ASTNode> makeNullableHashStmt(
            const FieldNode* field, const std::string& accumulatorVar)
        {
            // condition: this.f == null
            auto cond = makeBinary(
                makeThisField(field->getName()),
                TokenType::EQUALS,
                std::make_unique<NullNode>());

            // then: h = 31 * h + 0;
            auto thenAssign = std::make_unique<AssignmentNode>(
                accumulatorVar,
                makeBinary(
                    makeBinary(makeIntLiteral(31),
                               TokenType::MULTIPLY,
                               std::make_unique<VariableNode>(accumulatorVar)),
                    TokenType::PLUS,
                    makeIntLiteral(0)),
                ValueType::VOID, "", false, false);

            // else: h = 31 * h + this.f.hashCode();
            auto elseAssign = std::make_unique<AssignmentNode>(
                accumulatorVar,
                makeBinary(
                    makeBinary(makeIntLiteral(31),
                               TokenType::MULTIPLY,
                               std::make_unique<VariableNode>(accumulatorVar)),
                    TokenType::PLUS,
                    makeMethodCall(makeThisField(field->getName()), "hashCode")),
                ValueType::VOID, "", false, false);

            // Wrap each branch as a single-statement BlockNode so the parser-
            // tree shape matches normal user code.
            std::vector<std::unique_ptr<ASTNode>> thenStmts;
            thenStmts.push_back(std::move(thenAssign));
            std::vector<std::unique_ptr<ASTNode>> elseStmts;
            elseStmts.push_back(std::move(elseAssign));

            return std::make_unique<IfNode>(
                std::move(cond),
                std::make_unique<BlockNode>(std::move(thenStmts)),
                std::make_unique<BlockNode>(std::move(elseStmts)));
        }

        // Synthesize a single accumulating assignment for one field:
        //   h = 31 * h + <component>;
        std::unique_ptr<ASTNode> makeHashAccumulatorStmt(
            const FieldNode* field, const std::string& accumulatorVar)
        {
            auto component = makeHashComponent(field);
            auto rhs = makeBinary(
                makeBinary(makeIntLiteral(31),
                           TokenType::MULTIPLY,
                           std::make_unique<VariableNode>(accumulatorVar)),
                TokenType::PLUS,
                std::move(component));
            return std::make_unique<AssignmentNode>(
                accumulatorVar, std::move(rhs),
                ValueType::VOID, "", false, false);
        }

        // ---- equals component synthesis ---------------------------------

        // For each field, emit `if (<this.f != o.f>) return false;`.
        // NOTE: FLOAT/BOOL fall into the same fast `!=` branch below but
        // are unreachable in Phase 1 (Policy gates them out). Pre-staged
        // for Phase 1.5+ so this path doesn't need a follow-up edit when
        // the gate relaxes.
        std::unique_ptr<ASTNode> makeFieldInequalityEarlyOut(
            const FieldNode* field, const std::string& castedLocal)
        {
            FieldKind kind = classifyField(field);
            std::unique_ptr<ASTNode> condition;

            if (kind == FieldKind::INT || kind == FieldKind::FLOAT || kind == FieldKind::BOOL)
            {
                // condition: this.f != o.f
                condition = makeBinary(
                    makeThisField(field->getName()),
                    TokenType::NOT_EQUALS,
                    makeOtherField(castedLocal, field->getName()));
            }
            else
            {
                // OBJECT: !(this.f.equals(o.f)) — but null-guard if nullable.
                if (isFieldNullable(field))
                {
                    // If both null, equal (skip early-out). If one null,
                    // not equal (emit return false). Else compare via
                    // .equals(). To keep op count down we lower this as:
                    //   if (this.f == null) {
                    //       if (o.f != null) return false;
                    //   } else {
                    //       if (!this.f.equals(o.f)) return false;
                    //   }
                    auto thisIsNull = makeBinary(
                        makeThisField(field->getName()),
                        TokenType::EQUALS,
                        std::make_unique<NullNode>());
                    auto otherNotNull = makeBinary(
                        makeOtherField(castedLocal, field->getName()),
                        TokenType::NOT_EQUALS,
                        std::make_unique<NullNode>());

                    std::vector<std::unique_ptr<ASTNode>> innerThenStmts;
                    innerThenStmts.push_back(std::make_unique<ReturnNode>(
                        std::make_unique<BoolNode>(false)));
                    auto thenIf = std::make_unique<IfNode>(
                        std::move(otherNotNull),
                        std::make_unique<BlockNode>(std::move(innerThenStmts)));

                    std::vector<std::unique_ptr<ASTNode>> thenStmts;
                    thenStmts.push_back(std::move(thenIf));

                    auto eqCall = makeMethodCall1Arg(
                        makeThisField(field->getName()), "equals",
                        makeOtherField(castedLocal, field->getName()));
                    auto notEq = std::make_unique<UnaryExpNode>(TokenType::NOT, std::move(eqCall));
                    std::vector<std::unique_ptr<ASTNode>> innerElseThenStmts;
                    innerElseThenStmts.push_back(std::make_unique<ReturnNode>(
                        std::make_unique<BoolNode>(false)));
                    auto elseIf = std::make_unique<IfNode>(
                        std::move(notEq),
                        std::make_unique<BlockNode>(std::move(innerElseThenStmts)));

                    std::vector<std::unique_ptr<ASTNode>> elseStmts;
                    elseStmts.push_back(std::move(elseIf));

                    return std::make_unique<IfNode>(
                        std::move(thisIsNull),
                        std::make_unique<BlockNode>(std::move(thenStmts)),
                        std::make_unique<BlockNode>(std::move(elseStmts)));
                }
                // Non-nullable object: !this.f.equals(o.f)
                auto eqCall = makeMethodCall1Arg(
                    makeThisField(field->getName()), "equals",
                    makeOtherField(castedLocal, field->getName()));
                condition = std::make_unique<UnaryExpNode>(TokenType::NOT, std::move(eqCall));
            }

            std::vector<std::unique_ptr<ASTNode>> thenStmts;
            thenStmts.push_back(std::make_unique<ReturnNode>(
                std::make_unique<BoolNode>(false)));
            return std::make_unique<IfNode>(
                std::move(condition),
                std::make_unique<BlockNode>(std::move(thenStmts)));
        }
    } // anonymous namespace

    std::unique_ptr<MethodNode> StructuralEqualityCodegen::buildHashCodeMethod(
        const ClassNode* /*owner*/,
        const std::vector<const FieldNode*>& fields,
        bool composeWithSuper)
    {
        const std::string accVar = "__h";
        std::vector<std::unique_ptr<ASTNode>> stmts;

        // int __h = (composeWithSuper ? super.hashCode() : 1);
        std::unique_ptr<ASTNode> initValue;
        if (composeWithSuper)
        {
            std::vector<std::unique_ptr<ASTNode>> noArgs;
            initValue = std::make_unique<SuperMethodCallNode>("hashCode", std::move(noArgs));
        }
        else
        {
            initValue = makeIntLiteral(1);
        }
        stmts.push_back(std::make_unique<AssignmentNode>(
            accVar, std::move(initValue),
            ValueType::INT, /*className*/ "",
            /*isFinal*/ false, /*isStatic*/ false));

        for (const auto* field : fields)
        {
            FieldKind kind = classifyField(field);
            const bool nullable = (kind == FieldKind::OBJECT) && isFieldNullable(field);
            if (nullable)
            {
                stmts.push_back(makeNullableHashStmt(field, accVar));
            }
            else
            {
                stmts.push_back(makeHashAccumulatorStmt(field, accVar));
            }
        }

        // return __h;
        stmts.push_back(std::make_unique<ReturnNode>(
            std::make_unique<VariableNode>(accVar)));

        auto body = std::make_shared<BlockNode>(std::move(stmts));

        auto retType = std::make_shared<GenericType>(ValueType::INT);
        std::vector<std::pair<std::string, std::shared_ptr<GenericType>>> params;

        auto method = std::make_unique<MethodNode>(
            "hashCode", retType, params,
            std::static_pointer_cast<ASTNode>(body),
            /*isStatic*/ false,
            std::vector<GenericTypeParameter>{},
            AccessModifier::PUBLIC, /*async*/ false);
        method->setSynthetic(true);
        return method;
    }

    std::unique_ptr<MethodNode> StructuralEqualityCodegen::buildEqualsMethod(
        const ClassNode* owner,
        const std::vector<const FieldNode*>& fields,
        bool composeWithSuper)
    {
        const std::string castedLocal = "__o";
        std::vector<std::unique_ptr<ASTNode>> stmts;

        // if (!(other isClassOf <Owner>)) return false;
        auto ownerType = std::make_shared<GenericType>(owner->getClassName());
        auto instanceCheck = std::make_unique<InstanceOfExpression>(
            std::make_unique<VariableNode>("other"), ownerType);
        auto notInstance = std::make_unique<UnaryExpNode>(
            TokenType::NOT, std::move(instanceCheck));

        std::vector<std::unique_ptr<ASTNode>> earlyFalseStmts;
        earlyFalseStmts.push_back(std::make_unique<ReturnNode>(
            std::make_unique<BoolNode>(false)));
        stmts.push_back(std::make_unique<IfNode>(
            std::move(notInstance),
            std::make_unique<BlockNode>(std::move(earlyFalseStmts))));

        // if (composeWithSuper) { if (!super.equals(other)) return false; }
        // The parent's synthesized equals already checks `other isClassOf
        // Parent` (which any Owner instance satisfies, since Owner extends
        // Parent), then compares the parent's own fields. This composes
        // child checks on top.
        if (composeWithSuper)
        {
            std::vector<std::unique_ptr<ASTNode>> superArgs;
            superArgs.push_back(std::make_unique<VariableNode>("other"));
            auto superCall = std::make_unique<SuperMethodCallNode>("equals", std::move(superArgs));
            auto notSuper = std::make_unique<UnaryExpNode>(
                TokenType::NOT, std::move(superCall));
            std::vector<std::unique_ptr<ASTNode>> superFalseStmts;
            superFalseStmts.push_back(std::make_unique<ReturnNode>(
                std::make_unique<BoolNode>(false)));
            stmts.push_back(std::make_unique<IfNode>(
                std::move(notSuper),
                std::make_unique<BlockNode>(std::move(superFalseStmts))));
        }

        // <Owner> __o = (<Owner>)other;
        auto castType = std::make_shared<GenericType>(owner->getClassName());
        auto castExpr = std::make_unique<CastExpression>(
            std::make_unique<VariableNode>("other"), castType);
        stmts.push_back(std::make_unique<AssignmentNode>(
            castedLocal, std::move(castExpr),
            ValueType::OBJECT, owner->getClassName(),
            /*isFinal*/ false, /*isStatic*/ false));

        // For each field: if (this.f != __o.f) return false;
        for (const auto* field : fields)
        {
            stmts.push_back(makeFieldInequalityEarlyOut(field, castedLocal));
        }

        // return true;
        stmts.push_back(std::make_unique<ReturnNode>(std::make_unique<BoolNode>(true)));

        auto body = std::make_shared<BlockNode>(std::move(stmts));

        auto retType = std::make_shared<GenericType>(ValueType::BOOL);
        // Parameter: Object other — MUST match the inherited Object.equals
        // signature exactly so the synthesized method overrides cleanly.
        // Adding a nullable overload (Object?) would coexist alongside the
        // inherited (Object) version and create overload ambiguity at every
        // .equals() call site (including inside this synthesized body's own
        // recursive .equals() calls on object fields).
        auto otherType = std::make_shared<GenericType>(std::string{"Object"});
        std::vector<std::pair<std::string, std::shared_ptr<GenericType>>> params;
        params.emplace_back("other", otherType);

        auto method = std::make_unique<MethodNode>(
            "equals", retType, params,
            std::static_pointer_cast<ASTNode>(body),
            /*isStatic*/ false,
            std::vector<GenericTypeParameter>{},
            AccessModifier::PUBLIC, /*async*/ false);
        method->setSynthetic(true);
        return method;
    }
}
