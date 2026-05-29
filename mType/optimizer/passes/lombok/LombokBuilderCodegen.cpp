#include "LombokBuilderCodegen.hpp"
#include "LombokAstBuilders.hpp"

#include "../../../ast/GenericType.hpp"
#include "../../../ast/GenericTypeParameter.hpp"
#include "../../../ast/AccessModifier.hpp"
#include "../../../ast/nodes/classes/ClassNode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../../ast/nodes/classes/NewNode.hpp"

#include <string>
#include <utility>

namespace optimizer::passes::lombok
{
    using namespace ast;
    using namespace ast::nodes::classes;
    using namespace detail;
    using value::ValueType;

    namespace
    {
        using GenericParam = std::pair<std::string, std::shared_ptr<GenericType>>;

        std::unique_ptr<MethodNode> makeMethod(
            const std::string& name,
            std::shared_ptr<GenericType> retType,
            const std::vector<GenericParam>& params,
            std::shared_ptr<ASTNode> body,
            bool isStatic)
        {
            auto method = std::make_unique<MethodNode>(
                name, std::move(retType), params, std::move(body),
                isStatic,
                std::vector<GenericTypeParameter>{},
                AccessModifier::PUBLIC, /*async*/ false);
            method->setSynthetic(true);
            return method;
        }

        std::unique_ptr<ASTNode> makeNew(const std::string& className,
                                         std::vector<std::unique_ptr<ASTNode>> args)
        {
            return std::make_unique<NewNode>(className, std::move(args));
        }
    }

    std::unique_ptr<ClassNode> LombokBuilderCodegen::buildBuilderClass(
        const ClassNode* owner,
        const std::vector<const FieldNode*>& allFields)
    {
        const std::string builderName = builderClassName(owner->getClassName());
        auto builder = std::make_unique<ClassNode>(builderName);

        // One mutable field per target field, default-initialized (uninitialized
        // fields are legal in mType and read as null/zero until set fluently).
        for (const auto* f : allFields)
        {
            auto field = std::make_unique<FieldNode>(
                f->getName(), cloneFieldType(f), /*initValue*/ nullptr,
                /*isStatic*/ false, /*isFinal*/ false, AccessModifier::PRIVATE);
            builder->addField(std::move(field));
        }

        // Fluent setter per field: `<Builder> <field>(<T> <field>) { this.<field> = <field>; return this; }`
        for (const auto* f : allFields)
        {
            std::vector<std::unique_ptr<ASTNode>> stmts;
            stmts.push_back(makeThisFieldAssign(f->getName(), makeVar(f->getName())));
            stmts.push_back(makeReturn(makeThis()));

            std::vector<GenericParam> params;
            params.emplace_back(f->getName(), cloneFieldType(f));

            builder->addMethod(makeMethod(
                f->getName(),
                std::make_shared<GenericType>(builderName),
                params,
                makeBlock(std::move(stmts)),
                /*isStatic*/ false));
        }

        // `<Owner> build() { return new <Owner>(f0, f1, ...); }`
        {
            std::vector<std::unique_ptr<ASTNode>> ctorArgs;
            for (const auto* f : allFields)
                ctorArgs.push_back(makeThisField(f->getName()));

            std::vector<std::unique_ptr<ASTNode>> stmts;
            stmts.push_back(makeReturn(makeNew(owner->getClassName(), std::move(ctorArgs))));

            std::vector<GenericParam> params;
            builder->addMethod(makeMethod(
                "build",
                std::make_shared<GenericType>(owner->getClassName()),
                params,
                makeBlock(std::move(stmts)),
                /*isStatic*/ false));
        }

        return builder;
    }

    std::unique_ptr<MethodNode> LombokBuilderCodegen::buildBuilderFactory(
        const ClassNode* owner)
    {
        const std::string builderName = builderClassName(owner->getClassName());

        std::vector<std::unique_ptr<ASTNode>> noArgs;
        std::vector<std::unique_ptr<ASTNode>> stmts;
        stmts.push_back(makeReturn(makeNew(builderName, std::move(noArgs))));

        std::vector<GenericParam> params;
        return makeMethod(
            "builder",
            std::make_shared<GenericType>(builderName),
            params,
            makeBlock(std::move(stmts)),
            /*isStatic*/ true);
    }
}
