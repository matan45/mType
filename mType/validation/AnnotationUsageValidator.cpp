#include "AnnotationUsageValidator.hpp"
#include "../runtimeTypes/klass/AnnotationDefinition.hpp"
#include "../errors/TypeException.hpp"
#include <sstream>

namespace validation
{
    using namespace ast::nodes::annotations;
    using runtimeTypes::klass::AnnotationDefinition;
    using runtimeTypes::klass::AnnotationParamSchema;

    namespace
    {
        const char* typeName(AnnotationValueType t)
        {
            switch (t)
            {
            case AnnotationValueType::NULL_VALUE:  return "null";
            case AnnotationValueType::INT:         return "int";
            case AnnotationValueType::FLOAT:       return "float";
            case AnnotationValueType::BOOL:        return "bool";
            case AnnotationValueType::STRING:      return "string";
            case AnnotationValueType::CLASS_REF:   return "Class";
            case AnnotationValueType::CLASS_ARRAY: return "Class[]";
            }
            return "<unknown>";
        }

        // Allow some forgiving conversions:
        //   * INT literal accepted where FLOAT declared
        //   * CLASS_REF accepted where CLASS_ARRAY declared (single-element shorthand)
        bool isAssignable(AnnotationValueType actual, const AnnotationParamSchema& schema)
        {
            if (actual == schema.declaredType) return true;
            if (schema.declaredType == AnnotationValueType::FLOAT && actual == AnnotationValueType::INT) return true;
            if (schema.declaredType == AnnotationValueType::CLASS_ARRAY && actual == AnnotationValueType::CLASS_REF) return true;
            return false;
        }

        // Map an AnnotationHostKind to the marker-class name used inside
        // @Target([...]) arrays. Names match lib/annotations/Targets.mt.
        const char* hostKindMarker(AnnotationHostKind kind)
        {
            switch (kind)
            {
            case AnnotationHostKind::CLASS:                  return "CLASS";
            case AnnotationHostKind::METHOD:                 return "METHOD";
            case AnnotationHostKind::FIELD:                  return "FIELD";
            case AnnotationHostKind::CONSTRUCTOR:            return "CONSTRUCTOR";
            case AnnotationHostKind::FUNCTION:               return "FUNCTION";
            case AnnotationHostKind::PARAMETER:              return "PARAMETER";
            case AnnotationHostKind::ANNOTATION_DECLARATION: return "ANNOTATION";
            case AnnotationHostKind::UNSPECIFIED:            return nullptr;
            }
            return nullptr;
        }
    }

    void AnnotationUsageValidator::validate(
        std::shared_ptr<AnnotationNode> annotation,
        std::shared_ptr<environment::Environment> environment,
        const errors::SourceLocation& location,
        AnnotationHostKind hostKind)
    {
        if (!annotation || !environment) return;
        auto registry = environment->getAnnotationRegistry();
        if (!registry) return;

        auto def = registry->findAnnotation(annotation->getName());
        if (!def)
        {
            std::ostringstream oss;
            oss << "Unknown annotation '@" << annotation->getName() << "' — no annotation type with this name has been declared.";
            throw errors::TypeException(oss.str(), location);
        }

        // MYT-109: enforce @Target if the annotation's declaration carries one
        // and the caller supplied a host kind to check against.
        if (const char* marker = hostKindMarker(hostKind))
        {
            if (auto targetMeta = def->getMetaAnnotation("Target"))
            {
                const TypedAnnotationValue* targets =
                    targetMeta->getTypedParameter("targets");
                // Accept positional shorthand (`@Target([METHOD])`) which is
                // parsed under "__positional__" until AnnotationUsageValidator
                // rebinds it; meta-annotations themselves aren't re-validated
                // through this path, so read both keys.
                if (!targets) targets = targetMeta->getTypedParameter("__positional__");
                if (targets && targets->getType() == AnnotationValueType::CLASS_ARRAY)
                {
                    const auto& allowed = targets->asClassArray();
                    bool permitted = false;
                    for (const auto& t : allowed)
                    {
                        if (t == marker) { permitted = true; break; }
                    }
                    if (!permitted)
                    {
                        std::ostringstream oss;
                        oss << "Annotation '@" << annotation->getName()
                            << "' cannot be applied to a " << marker
                            << " — allowed targets: [";
                        for (size_t i = 0; i < allowed.size(); ++i)
                        {
                            if (i) oss << ", ";
                            oss << allowed[i];
                        }
                        oss << "].";
                        throw errors::TypeException(oss.str(), location);
                    }
                }
                else if (targets && targets->getType() == AnnotationValueType::CLASS_REF)
                {
                    // Single-class shorthand: `@Target(METHOD)`.
                    if (targets->asClassRef() != marker)
                    {
                        std::ostringstream oss;
                        oss << "Annotation '@" << annotation->getName()
                            << "' cannot be applied to a " << marker
                            << " — allowed target: " << targets->asClassRef() << ".";
                        throw errors::TypeException(oss.str(), location);
                    }
                }
            }
        }

        // Resolve positional shorthand → bind to sole declared param.
        if (annotation->hasParameter("__positional__"))
        {
            const auto& params = def->getParams();
            if (params.size() != 1)
            {
                std::ostringstream oss;
                oss << "Positional shorthand only allowed on annotations with exactly 1 declared parameter; "
                    << "'@" << annotation->getName() << "' declares " << params.size() << ".";
                throw errors::TypeException(oss.str(), location);
            }
            const TypedAnnotationValue* posValue = annotation->getTypedParameter("__positional__");
            if (posValue)
            {
                TypedAnnotationValue copy = *posValue;
                annotation->setTypedParameter(params[0].name, std::move(copy));
                annotation->removeTypedParameter("__positional__");
            }
        }

        // Migrate legacy @Throw bare-list — parser stores under "exceptions"
        // already, so nothing to do for the named case. But if the legacy
        // form was used and the schema's param name differs (shouldn't, since
        // built-in @Throw uses "exceptions"), the validator below will catch.

        // Walk every supplied parameter and check name + type.
        for (const auto& key : annotation->getKeyOrder())
        {
            if (key == "__positional__") continue; // synthetic key
            const TypedAnnotationValue* value = annotation->getTypedParameter(key);
            if (!value) continue;

            const AnnotationParamSchema* schema = def->findParam(key);
            if (!schema)
            {
                std::ostringstream oss;
                oss << "Unknown parameter '" << key << "' for annotation '@" << annotation->getName() << "'.";
                throw errors::TypeException(oss.str(), location);
            }

            if (value->isNull())
            {
                if (!schema->nullable)
                {
                    std::ostringstream oss;
                    oss << "Annotation '@" << annotation->getName() << "' parameter '" << key
                        << "' is not nullable but received null.";
                    throw errors::TypeException(oss.str(), location);
                }
                continue;
            }

            if (!isAssignable(value->getType(), *schema))
            {
                std::ostringstream oss;
                oss << "Annotation '@" << annotation->getName() << "' parameter '" << key
                    << "' expects " << typeName(schema->declaredType)
                    << " but received " << typeName(value->getType()) << ".";
                throw errors::TypeException(oss.str(), location);
            }
        }

        // Fill defaults + check missing required.
        for (const auto& schema : def->getParams())
        {
            if (annotation->hasParameter(schema.name)) continue;
            if (schema.defaultValue.has_value())
            {
                annotation->setTypedParameter(schema.name, *schema.defaultValue);
                continue;
            }
            std::ostringstream oss;
            oss << "Annotation '@" << annotation->getName() << "' is missing required parameter '"
                << schema.name << "'.";
            throw errors::TypeException(oss.str(), location);
        }
    }
}
