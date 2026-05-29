#include "BuiltInAnnotations.hpp"
#include "../../environment/registry/AnnotationDefinition.hpp"
#include "../../environment/registry/AnnotationParamSchema.hpp"

namespace validation::builtins
{
    using namespace runtimeTypes::klass;
    using ast::nodes::annotations::AnnotationValueType;

    void BuiltInAnnotations::registerAll(std::shared_ptr<environment::registry::AnnotationRegistry> registry)
    {
        if (!registry) return;

        // @Override — zero parameters. Lifecycle/signature checks live in
        // AnnotationValidator::validateOverrideAnnotation (unchanged).
        if (!registry->hasAnnotation("Override"))
        {
            auto def = std::make_shared<AnnotationDefinition>("Override", true);
            registry->registerAnnotation("Override", def);
        }

        // @Script — zero parameters. Per explicit user requirement, the
        // semantic enforcement (default ctor + onStart/onUpdate(float)/onDestroy)
        // remains in C++ in AnnotationValidator::validateScriptAnnotation.
        // @Script is a lifecycle marker that only makes sense on a class
        // (the validator looks for constructors and onStart/onUpdate/onDestroy
        // methods), so @Target restricts it to CLASS.
        if (!registry->hasAnnotation("Script"))
        {
            auto def = std::make_shared<AnnotationDefinition>("Script", true);

            auto targetMeta = std::make_shared<ast::nodes::annotations::AnnotationNode>("Target");
            targetMeta->setTypedParameter(
                "targets",
                ast::nodes::annotations::TypedAnnotationValue::makeClassArray({"CLASS"}));
            def->addMetaAnnotation(targetMeta);

            registry->registerAnnotation("Script", def);
        }

        // @EntryPoint — zero parameters. Validator checks for static main().
        if (!registry->hasAnnotation("EntryPoint"))
        {
            auto def = std::make_shared<AnnotationDefinition>("EntryPoint", true);
            registry->registerAnnotation("EntryPoint", def);
        }

        // @Throw(exceptions: Class[]) — migrated to typed Class[] parameter.
        // Both new form `@Throw(exceptions = [IO, Net])` and legacy bare-id form
        // `@Throw(IO, Net)` parse to the same internal storage (parser stashes
        // legacy form under "exceptions" as CLASS_ARRAY too).
        if (!registry->hasAnnotation("Throw"))
        {
            auto def = std::make_shared<AnnotationDefinition>("Throw", true);
            AnnotationParamSchema schema;
            schema.name         = "exceptions";
            schema.declaredType = AnnotationValueType::CLASS_ARRAY;
            schema.nullable     = false;
            schema.isArray      = true;
            def->addParam(std::move(schema));

            // @Throw is only meaningful where a body executes user code that
            // can raise: instance methods and free functions. Reject it on
            // fields, constructors, parameters, classes, and annotations.
            auto targetMeta = std::make_shared<ast::nodes::annotations::AnnotationNode>("Target");
            targetMeta->setTypedParameter(
                "targets",
                ast::nodes::annotations::TypedAnnotationValue::makeClassArray({"METHOD", "FUNCTION"}));
            def->addMetaAnnotation(targetMeta);

            registry->registerAnnotation("Throw", def);
        }

        // MYT-109 (3a) — meta-annotations.

        // @Retention(policy: Class) — controls whether the annotation survives
        // into bytecode. Policy is one of the marker classes SOURCE / RUNTIME
        // (lib/annotations/RetentionPolicy.mt). Positional shorthand is
        // accepted: `@Retention(RUNTIME)`.
        if (!registry->hasAnnotation("Retention"))
        {
            auto def = std::make_shared<AnnotationDefinition>("Retention", true);
            AnnotationParamSchema schema;
            schema.name         = "policy";
            schema.declaredType = AnnotationValueType::CLASS_REF;
            schema.nullable     = false;
            schema.isArray      = false;
            def->addParam(std::move(schema));
            registry->registerAnnotation("Retention", def);
        }

        // @Target(targets: Class[]) — restricts the host kinds an annotation
        // may be applied to. Values are marker classes from lib/annotations/
        // (Method, Field, Constructor, ClassTarget, FunctionTarget,
        // AnnotationTarget). Positional shorthand: `@Target([METHOD])`.
        if (!registry->hasAnnotation("Target"))
        {
            auto def = std::make_shared<AnnotationDefinition>("Target", true);
            AnnotationParamSchema schema;
            schema.name         = "targets";
            schema.declaredType = AnnotationValueType::CLASS_ARRAY;
            schema.nullable     = false;
            schema.isArray      = true;
            def->addParam(std::move(schema));
            registry->registerAnnotation("Target", def);
        }

        // @Inherited — zero-parameter marker. When applied to an annotation
        // declaration, class-level instances of that annotation are inherited
        // by subclasses.
        if (!registry->hasAnnotation("Inherited"))
        {
            auto def = std::make_shared<AnnotationDefinition>("Inherited", true);
            registry->registerAnnotation("Inherited", def);
        }

        // ---- Lombok-style synthesis markers --------------------------------
        // Zero-parameter compile-time markers consumed by the optimizer's
        // LombokSynthesisPass (which injects the generated members before
        // bytecode compilation). They carry no @Retention, so they are dropped
        // before class registration like other compile-time directives. The
        // synthesis itself is C++ (mType has no runtime metaprogramming); these
        // declarations only restrict where the markers may be applied.
        registerMarker(registry, "Getter", {"CLASS", "FIELD"});
        registerMarker(registry, "Setter", {"CLASS", "FIELD"});
        registerMarker(registry, "ToString", {"CLASS"});
        registerMarker(registry, "NoArgsConstructor", {"CLASS"});
        registerMarker(registry, "AllArgsConstructor", {"CLASS"});
        registerMarker(registry, "EqualsAndHashCode", {"CLASS"});
        registerMarker(registry, "Data", {"CLASS"});
        registerMarker(registry, "Builder", {"CLASS"});
    }

    void BuiltInAnnotations::registerMarker(
        std::shared_ptr<environment::registry::AnnotationRegistry> registry,
        const std::string& name,
        const std::vector<std::string>& targets)
    {
        if (registry->hasAnnotation(name)) return;

        auto def = std::make_shared<AnnotationDefinition>(name, true);

        auto targetMeta = std::make_shared<ast::nodes::annotations::AnnotationNode>("Target");
        targetMeta->setTypedParameter(
            "targets",
            ast::nodes::annotations::TypedAnnotationValue::makeClassArray(targets));
        def->addMetaAnnotation(targetMeta);

        registry->registerAnnotation(name, def);
    }
}
