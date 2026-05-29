#include "LombokSynthesisPass.hpp"
#include <chrono>

#include "lombok/LombokCodegen.hpp"
#include "lombok/LombokBuilderCodegen.hpp"
#include "lombok/LombokAstBuilders.hpp"

#include "../OptimizationResult.hpp"
#include "../base/OptimizationContext.hpp"
#include "../../ast/nodes/classes/ClassNode.hpp"
#include "../../ast/nodes/classes/FieldNode.hpp"
#include "../../ast/nodes/classes/MethodNode.hpp"
#include "../../ast/nodes/classes/ConstructorNode.hpp"
#include "../../ast/nodes/statements/ProgramNode.hpp"

namespace optimizer::passes
{
    using namespace ast;
    using ast::nodes::statements::ProgramNode;
    using lombok::ClassRegistry;
    using lombok::LombokBuilderCodegen;
    using lombok::LombokCodegen;
    using lombok::LombokPolicy;

    namespace
    {
        ClassRegistry buildClassRegistry(const ProgramNode* program)
        {
            ClassRegistry registry;
            for (const auto& stmt : program->getStatements())
            {
                if (auto* classNode = dynamic_cast<ClassNode*>(stmt.get()))
                {
                    registry[classNode->getClassName()] = classNode;
                }
            }
            return registry;
        }

        std::vector<const FieldNode*> concatFields(
            const std::vector<const FieldNode*>& a,
            const std::vector<const FieldNode*>& b)
        {
            std::vector<const FieldNode*> out;
            out.reserve(a.size() + b.size());
            for (const auto* f : a) out.push_back(f);
            for (const auto* f : b) out.push_back(f);
            return out;
        }
    }

    LombokSynthesisPass::LombokSynthesisPass()
        : OptimizationPass("LombokSynthesis", base::PassType::TRANSFORMATION)
    {
        dependencies = {};
    }

    void LombokSynthesisPass::processClass(
        ClassNode* classNode,
        const ClassRegistry& registry,
        std::vector<std::unique_ptr<ASTNode>>& newClasses,
        std::unordered_set<std::string>& queuedBuilders)
    {
        if (LombokPolicy::isShapeSkippable(classNode)) return;

        const bool data = classNode->hasAnnotation("Data");
        const bool wantGetter = data || classNode->hasAnnotation("Getter");
        const bool wantSetter = data || classNode->hasAnnotation("Setter");
        const bool wantToString = data || classNode->hasAnnotation("ToString");
        const bool wantNoArgs = classNode->hasAnnotation("NoArgsConstructor");
        const bool wantAllArgs = classNode->hasAnnotation("AllArgsConstructor");
        const bool wantBuilder = classNode->hasAnnotation("Builder");

        if (!(data || wantGetter || wantSetter || wantToString ||
              wantNoArgs || wantAllArgs || wantBuilder))
        {
            return;
        }

        const auto own = LombokPolicy::collectOwnInstanceFields(classNode);
        const auto inherited = LombokPolicy::collectInheritedInstanceFields(classNode, registry);
        const auto allFields = concatFields(inherited, own);

        // --- @Getter ---
        if (wantGetter)
        {
            for (const auto* f : own)
            {
                if (LombokPolicy::hasMethod(classNode, lombok::detail::getterName(f->getName()), 0))
                    continue;
                classNode->addMethod(LombokCodegen::buildGetter(f));
                ++methodsSynthesized;
            }
        }

        // --- @Setter (skips final fields) ---
        if (wantSetter)
        {
            for (const auto* f : own)
            {
                if (f->getIsFinal()) continue;
                if (LombokPolicy::hasMethod(classNode, lombok::detail::setterName(f->getName()), 1))
                    continue;
                classNode->addMethod(LombokCodegen::buildSetter(f));
                ++methodsSynthesized;
            }
        }

        // --- @ToString ---
        if (wantToString && !LombokPolicy::hasMethod(classNode, "toString", 0))
        {
            classNode->addMethod(LombokCodegen::buildToString(classNode, allFields));
            ++methodsSynthesized;
        }

        // --- @Data required-args constructor (own final fields) ---
        if (data)
        {
            std::vector<const FieldNode*> required;
            for (const auto* f : own)
                if (f->getIsFinal()) required.push_back(f);

            const bool shouldGenerate = !required.empty() || classNode->getConstructorCount() == 0;
            if (shouldGenerate)
            {
                auto ptypes = LombokPolicy::toParameterTypes(required);
                if (!LombokPolicy::hasConstructorWithSignature(classNode, ptypes))
                {
                    classNode->addConstructor(LombokCodegen::buildArgsConstructor(required));
                    ++constructorsSynthesized;
                }
            }
        }

        // --- @NoArgsConstructor ---
        if (wantNoArgs && !LombokPolicy::hasNoArgConstructor(classNode))
        {
            classNode->addConstructor(
                LombokCodegen::buildNoArgsConstructor(classNode->hasParentClass()));
            ++constructorsSynthesized;
        }

        // --- @AllArgsConstructor ---
        if (wantAllArgs)
        {
            auto ptypes = LombokPolicy::toParameterTypes(allFields);
            if (!LombokPolicy::hasConstructorWithSignature(classNode, ptypes))
            {
                classNode->addConstructor(
                    LombokCodegen::buildAllArgsConstructor(inherited, own));
                ++constructorsSynthesized;
            }
        }

        // --- @Builder ---
        if (wantBuilder)
        {
            // build() calls the all-args constructor; ensure it exists.
            auto ptypes = LombokPolicy::toParameterTypes(allFields);
            if (!LombokPolicy::hasConstructorWithSignature(classNode, ptypes))
            {
                classNode->addConstructor(
                    LombokCodegen::buildAllArgsConstructor(inherited, own));
                ++constructorsSynthesized;
            }

            if (!LombokPolicy::hasMethod(classNode, "builder", 0))
            {
                classNode->addMethod(LombokBuilderCodegen::buildBuilderFactory(classNode));
                ++methodsSynthesized;
            }

            const std::string builderName =
                LombokBuilderCodegen::builderClassName(classNode->getClassName());
            const bool exists = registry.find(builderName) != registry.end() ||
                                queuedBuilders.find(builderName) != queuedBuilders.end();
            if (!exists)
            {
                newClasses.push_back(
                    LombokBuilderCodegen::buildBuilderClass(classNode, allFields));
                queuedBuilders.insert(builderName);
                ++builderClassesSynthesized;
            }
        }
    }

    std::unique_ptr<ASTNode> LombokSynthesisPass::optimize(
        std::unique_ptr<ASTNode> node,
        base::OptimizationContext& context)
    {
        methodsSynthesized = 0;
        constructorsSynthesized = 0;
        builderClassesSynthesized = 0;
        transformationCount = 0;

        auto startTime = std::chrono::high_resolution_clock::now();

        auto* program = dynamic_cast<ProgramNode*>(node.get());
        if (!program)
        {
            executionTime = std::chrono::milliseconds(0);
            return node;
        }

        ClassRegistry registry = buildClassRegistry(program);

        std::vector<std::unique_ptr<ASTNode>> newClasses;
        std::unordered_set<std::string> queuedBuilders;

        for (const auto& stmt : program->getStatements())
        {
            auto* classNode = dynamic_cast<ClassNode*>(stmt.get());
            if (!classNode) continue;
            processClass(classNode, registry, newClasses, queuedBuilders);
        }

        // Append companion builder classes after the walk (iterator safety).
        for (auto& cls : newClasses)
        {
            program->addStatement(std::move(cls));
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        executionTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        transformationCount = methodsSynthesized + constructorsSynthesized + builderClassesSynthesized;

        if (transformationCount > 0)
        {
            context.setModified(true);
        }

        return node;
    }

    std::string LombokSynthesisPass::getDescription() const
    {
        return "Lombok Synthesis: auto-generates getters/setters, toString, "
               "constructors, and builder classes from @Data/@Getter/@Setter/"
               "@ToString/@NoArgsConstructor/@AllArgsConstructor/@Builder marker "
               "annotations. equals/hashCode are delegated to the downstream "
               "StructuralEqualitySynthesis pass.";
    }

    void LombokSynthesisPass::reportMetrics(OptimizationResult& result) const
    {
        PassMetrics metrics(
            getName(),
            transformationCount,
            executionTime,
            transformationCount > 0);
        result.addPassMetrics(metrics);
    }

    void LombokSynthesisPass::reset()
    {
        methodsSynthesized = 0;
        constructorsSynthesized = 0;
        builderClassesSynthesized = 0;
        transformationCount = 0;
        executionTime = std::chrono::milliseconds(0);
    }
}
