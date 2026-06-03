#pragma once

#include "../../../ast/ASTNode.hpp"

namespace optimizer::passes::annotation_folding
{
    /// MYT-376: Compile-time resolver for runtime-evaluated annotation
    /// parameter values.
    ///
    /// The parser stores annotation arguments that are constant expressions or
    /// `Class::FIELD` static-final references as deferred expression ASTs on
    /// the AnnotationNode (see AnnotationParser). This pass walks the source
    /// AST (including resolved imports), builds a compile-time constant table
    /// from qualifying `static final` fields, folds every deferred expression
    /// to a literal `TypedAnnotationValue`, and clears the deferred map.
    ///
    /// It MUST run before class registration / bytecode serialization so the
    /// serializer and reflection only ever see plain literal values. Folding
    /// is Java-style: only literal constant expressions and references to
    /// `static final` fields with constant-foldable initializers are allowed;
    /// any genuinely runtime form (`new`, calls, instance access) throws a
    /// precise error.
    class AnnotationConstantResolver
    {
    public:
        // Folds all deferred annotation expressions reachable from `root`
        // (recursing into resolved imports). Throws errors::TypeException with
        // a precise message on any non-foldable / unresolved form.
        static void resolve(ast::ASTNode* root);
    };
}
