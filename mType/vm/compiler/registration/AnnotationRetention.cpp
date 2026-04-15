#include "AnnotationRetention.hpp"
#include "../../../runtimeTypes/klass/AnnotationDefinition.hpp"
#include "../../../ast/nodes/annotations/TypedAnnotationValue.hpp"

namespace vm::compiler::registration
{
    void populateAnnotationDataShared(
        bytecode::BytecodeProgram::AnnotationData& out,
        const ast::nodes::annotations::AnnotationNode& node)
    {
        using ast::nodes::annotations::AnnotationValueType;
        out.name = node.getName();
        out.location = node.getLocation();
        for (const auto& key : node.getKeyOrder())
        {
            const auto* v = node.getTypedParameter(key);
            if (!v) continue;
            bytecode::BytecodeProgram::TypedAnnotationArg arg;
            arg.key = key;
            arg.valueType = static_cast<uint8_t>(v->getType());
            switch (v->getType())
            {
            case AnnotationValueType::INT:         arg.intVal    = v->asInt(); break;
            case AnnotationValueType::FLOAT:       arg.floatVal  = v->asFloat(); break;
            case AnnotationValueType::BOOL:        arg.boolVal   = v->asBool(); break;
            case AnnotationValueType::STRING:      arg.stringVal = v->asString(); break;
            case AnnotationValueType::CLASS_REF:   arg.stringVal = v->asClassRef(); break;
            case AnnotationValueType::CLASS_ARRAY: arg.arrayVal  = v->asClassArray(); break;
            case AnnotationValueType::NULL_VALUE:  break;
            }
            out.typedArguments.push_back(std::move(arg));
        }
    }

    bool shouldRetainAnnotation(
        const ast::nodes::annotations::AnnotationNode& annotation,
        const std::shared_ptr<environment::Environment>& environment)
    {
        using ast::nodes::annotations::AnnotationValueType;

        if (!environment) return true;
        auto registry = environment->getAnnotationRegistry();
        if (!registry) return true;
        auto def = registry->findAnnotation(annotation.getName());
        if (!def) return true;
        auto retention = def->getMetaAnnotation("Retention");
        if (!retention) return true;

        for (const char* key : {"policy", "__positional__"})
        {
            if (const auto* v = retention->getTypedParameter(key))
            {
                return !(v->getType() == AnnotationValueType::CLASS_REF &&
                         v->asClassRef() == "SOURCE");
            }
        }
        return true;
    }
}
