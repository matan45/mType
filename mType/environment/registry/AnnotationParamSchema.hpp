#pragma once

#include "../../ast/nodes/annotations/TypedAnnotationValue.hpp"
#include <optional>
#include <string>

namespace runtimeTypes::klass
{
    /// Declared shape of a single annotation parameter:
    /// `int x = 5` becomes { name: "x", declaredType: INT, defaultValue: 5, nullable: false }.
    struct AnnotationParamSchema
    {
        std::string name;
        ast::nodes::annotations::AnnotationValueType declaredType =
            ast::nodes::annotations::AnnotationValueType::NULL_VALUE;
        std::optional<ast::nodes::annotations::TypedAnnotationValue> defaultValue;
        bool nullable = false;   // set when declared `Type? name`
        bool isArray  = false;   // set when declared `Type[] name` (v1: Class[] only)
    };
}
