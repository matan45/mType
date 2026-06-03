#pragma once
#include "../../ASTNode.hpp"
#include "TypedAnnotationValue.hpp"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

namespace ast::nodes::annotations
{
    /// AST node for an `@Annotation(args...)` usage on a class/method/field.
    /// Parameter values are stored as TypedAnnotationValue; the legacy
    /// string-only API is preserved as a back-compat formatting wrapper.
    class AnnotationNode : public ASTNode
    {
    private:
        std::string name;
        std::unordered_map<std::string, TypedAnnotationValue> typedParameters;
        // Insertion order of keys — used by the validator to support
        // positional shorthand (single-value annotations like `@DisplayName("x")`).
        std::vector<std::string> keyOrder;
        // MYT-376: transient expression-valued arguments that cannot be folded
        // at parse time (they reference `Class::FIELD` constants or are
        // constant expressions). The AnnotationConstantResolver pass folds each
        // to a TypedAnnotationValue and clears this map BEFORE bytecode
        // serialization, so it is never serialized and the literal-only
        // validator/serializer/reflection paths are unaffected.
        std::unordered_map<std::string, std::shared_ptr<ASTNode>> deferredExpressions;

    public:
        explicit AnnotationNode(const std::string& annotationName,
                                const SourceLocation& loc = SourceLocation());

        // Legacy constructor — accepts a string-only parameter map. Each value
        // is stored internally as a STRING-typed TypedAnnotationValue so that
        // existing producers (validators that haven't been migrated yet) keep
        // working through the legacy `setParameter`/`getParameter` API.
        AnnotationNode(const std::string& annotationName,
                       const std::unordered_map<std::string, std::string>& params,
                       const SourceLocation& loc = SourceLocation());

        const std::string& getName() const;
        void setName(const std::string& annotationName);

        // Typed accessors (preferred for new code).
        void setTypedParameter(const std::string& key, TypedAnnotationValue value);
        const TypedAnnotationValue* getTypedParameter(const std::string& key) const;
        const std::unordered_map<std::string, TypedAnnotationValue>& getTypedParameters() const;
        const std::vector<std::string>& getKeyOrder() const { return keyOrder; }
        // Removes a typed parameter (from both map and keyOrder).
        // Returns true if the key was present. Used by the usage validator to
        // erase the synthetic "__positional__" key after binding it to the
        // sole declared parameter name.
        bool removeTypedParameter(const std::string& key);

        // MYT-376: deferred expression-valued arguments. Stored by the parser
        // when an argument is a constant expression / `Class::FIELD` reference;
        // consumed and cleared by AnnotationConstantResolver before
        // serialization. `key` may be a declared parameter name or the
        // synthetic "__positional__" key (resolved like the typed path).
        void setDeferredExpression(const std::string& key, std::shared_ptr<ASTNode> expr);
        const std::unordered_map<std::string, std::shared_ptr<ASTNode>>& getDeferredExpressions() const;
        bool hasDeferredExpressions() const;
        void clearDeferredExpressions();

        // Legacy string-only API — preserved so unmigrated code paths keep
        // building. Reads format the typed value via `toDisplayString()`;
        // writes wrap the string in a STRING-typed TypedAnnotationValue.
        std::unordered_map<std::string, std::string> getParameters() const;
        std::string getParameter(const std::string& key, const std::string& defaultValue = "") const;
        bool hasParameter(const std::string& key) const;
        void setParameter(const std::string& key, const std::string& value);

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
