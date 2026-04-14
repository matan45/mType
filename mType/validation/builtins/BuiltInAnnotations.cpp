#include "BuiltInAnnotations.hpp"
#include "../../runtimeTypes/klass/AnnotationDefinition.hpp"
#include "../../runtimeTypes/klass/AnnotationParamSchema.hpp"

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
        // Registering here only gives the new usage validator a schema to
        // recognize the annotation as known.
        if (!registry->hasAnnotation("Script"))
        {
            auto def = std::make_shared<AnnotationDefinition>("Script", true);
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
            registry->registerAnnotation("Throw", def);
        }
    }
}
