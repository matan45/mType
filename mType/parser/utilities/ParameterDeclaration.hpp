#pragma once
#include <string>
#include <vector>
#include <memory>
#include "../../value/ValueType.hpp"
#include "../../value/ParameterType.hpp"
#include "../../ast/nodes/annotations/AnnotationNode.hpp"

namespace ast
{
    class GenericType;
}

namespace parser
{
    // MYT-110: carries a single parameter (name + type + per-parameter annotations)
    // through the parser -> AST -> runtime pipeline. Two flavors mirror the
    // existing three ParameterParser variants, which differ only in the type
    // representation they produce.

    struct ParameterDeclaration
    {
        std::string name;
        std::shared_ptr<ast::GenericType> type;
        std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>> annotations;
    };

    struct TypedParameterDeclaration
    {
        std::string name;
        value::ParameterType type;
        std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>> annotations;
    };

    struct ValueParameterDeclaration
    {
        std::string name;
        value::ValueType type;
        std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>> annotations;
    };
}
