#include "LombokCodegen.hpp"
#include "LombokAstBuilders.hpp"

#include "../../../ast/GenericType.hpp"
#include "../../../ast/GenericTypeParameter.hpp"
#include "../../../ast/AccessModifier.hpp"
#include "../../../ast/nodes/classes/ClassNode.hpp"
#include "../../../ast/nodes/classes/ConstructorNode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../../ast/nodes/classes/SuperConstructorCallNode.hpp"

#include <string>
#include <utility>

namespace optimizer::passes::lombok
{
    using namespace ast;
    using namespace ast::nodes::classes;
    using namespace detail;
    using value::ParameterType;
    using value::ValueType;

    namespace
    {
        using GenericParam = std::pair<std::string, std::shared_ptr<GenericType>>;

        std::unique_ptr<MethodNode> makePublicMethod(
            const std::string& name,
            std::shared_ptr<GenericType> retType,
            const std::vector<GenericParam>& params,
            std::shared_ptr<ASTNode> body)
        {
            // NOTE: intentionally NOT marked synthetic. Lombok-generated
            // members are real public API (reflection-visible, like Java
            // Lombok). The synthetic flag would route them into
            // ClassDefinition::syntheticInstanceMethods, which the
            // compile-time method-existence validator does not consult — so
            // calls to them would fail type-checking. (MYT-274's
            // equals/hashCode avoid that only because they resolve via Object
            // inheritance.)
            return std::make_unique<MethodNode>(
                name, std::move(retType), params, std::move(body),
                /*isStatic*/ false,
                std::vector<GenericTypeParameter>{},
                AccessModifier::PUBLIC, /*async*/ false);
        }

        // Builds the `name=<value>` expression piece for one field in toString.
        std::unique_ptr<ASTNode> toStringFieldValue(const FieldNode* field)
        {
            if (isStringConcatPrimitive(field))
            {
                return makeThisField(field->getName());
            }
            // Object / array / parameterized: render via .toString(), but guard
            // against null — `null.toString()` dereferences null and crashes at
            // runtime. Match Lombok and render the literal "null" instead:
            //   this.<f> == null ? "null" : this.<f>.toString()
            return makeTernary(
                makeIsNull(makeThisField(field->getName())),
                makeStringLit("null"),
                makeMethodCall0(makeThisField(field->getName()), "toString"));
        }
    }

    std::unique_ptr<MethodNode> LombokCodegen::buildGetter(const FieldNode* field)
    {
        std::vector<std::unique_ptr<ASTNode>> stmts;
        stmts.push_back(makeReturn(makeThisField(field->getName())));

        std::vector<GenericParam> params;
        return makePublicMethod(
            getterName(field->getName()),
            cloneFieldType(field),
            params,
            makeBlock(std::move(stmts)));
    }

    std::unique_ptr<MethodNode> LombokCodegen::buildSetter(const FieldNode* field)
    {
        std::vector<std::unique_ptr<ASTNode>> stmts;
        stmts.push_back(makeThisFieldAssign(field->getName(), makeVar("value")));

        std::vector<GenericParam> params;
        params.emplace_back("value", cloneFieldType(field));

        return makePublicMethod(
            setterName(field->getName()),
            std::make_shared<GenericType>(ValueType::VOID),
            params,
            makeBlock(std::move(stmts)));
    }

    std::unique_ptr<MethodNode> LombokCodegen::buildToString(
        const ClassNode* owner,
        const std::vector<const FieldNode*>& allFields)
    {
        // Build the left-folded concatenation:
        //   "<Class>(" + f0name + "=" + this.f0 + ", " + f1name + "=" + this.f1 + ")"
        std::unique_ptr<ASTNode> acc = makeStringLit(owner->getClassName() + "(");

        for (size_t i = 0; i < allFields.size(); ++i)
        {
            const FieldNode* field = allFields[i];
            std::string prefix = (i == 0 ? "" : ", ") + field->getName() + "=";
            acc = makeBinaryPlus(std::move(acc), makeStringLit(prefix));
            acc = makeBinaryPlus(std::move(acc), toStringFieldValue(field));
        }
        acc = makeBinaryPlus(std::move(acc), makeStringLit(")"));

        std::vector<std::unique_ptr<ASTNode>> stmts;
        stmts.push_back(makeReturn(std::move(acc)));

        std::vector<GenericParam> params;
        return makePublicMethod(
            "toString",
            std::make_shared<GenericType>(ValueType::STRING),
            params,
            makeBlock(std::move(stmts)));
    }

    std::unique_ptr<ConstructorNode> LombokCodegen::buildNoArgsConstructor(bool hasParent)
    {
        std::vector<std::pair<std::string, ParameterType>> params;
        std::vector<std::unique_ptr<ASTNode>> stmts;
        auto body = makeBlock(std::move(stmts));

        auto ctor = std::make_unique<ConstructorNode>(
            std::move(params), std::move(body), AccessModifier::PUBLIC);

        if (hasParent)
        {
            std::vector<std::unique_ptr<ASTNode>> superArgs;
            ctor->setSuperInitializer(
                std::make_unique<SuperConstructorCallNode>(std::move(superArgs)));
        }
        return ctor;
    }

    std::unique_ptr<ConstructorNode> LombokCodegen::buildAllArgsConstructor(
        const std::vector<const FieldNode*>& inheritedFields,
        const std::vector<const FieldNode*>& ownFields)
    {
        std::vector<std::pair<std::string, ParameterType>> params;
        params.reserve(inheritedFields.size() + ownFields.size());
        for (const auto* f : inheritedFields)
            params.emplace_back(f->getName(), fieldToParameterType(f));
        for (const auto* f : ownFields)
            params.emplace_back(f->getName(), fieldToParameterType(f));

        std::vector<std::unique_ptr<ASTNode>> stmts;
        for (const auto* f : ownFields)
            stmts.push_back(makeThisFieldAssign(f->getName(), makeVar(f->getName())));
        auto body = makeBlock(std::move(stmts));

        auto ctor = std::make_unique<ConstructorNode>(
            std::move(params), std::move(body), AccessModifier::PUBLIC);

        if (!inheritedFields.empty())
        {
            std::vector<std::unique_ptr<ASTNode>> superArgs;
            for (const auto* f : inheritedFields)
                superArgs.push_back(makeVar(f->getName()));
            ctor->setSuperInitializer(
                std::make_unique<SuperConstructorCallNode>(std::move(superArgs)));
        }
        return ctor;
    }

    std::unique_ptr<ConstructorNode> LombokCodegen::buildArgsConstructor(
        const std::vector<const FieldNode*>& fields)
    {
        std::vector<std::pair<std::string, ParameterType>> params;
        params.reserve(fields.size());
        for (const auto* f : fields)
            params.emplace_back(f->getName(), fieldToParameterType(f));

        std::vector<std::unique_ptr<ASTNode>> stmts;
        for (const auto* f : fields)
            stmts.push_back(makeThisFieldAssign(f->getName(), makeVar(f->getName())));

        return std::make_unique<ConstructorNode>(
            std::move(params), makeBlock(std::move(stmts)), AccessModifier::PUBLIC);
    }
}
