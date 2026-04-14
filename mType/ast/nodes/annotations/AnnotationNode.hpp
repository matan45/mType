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
