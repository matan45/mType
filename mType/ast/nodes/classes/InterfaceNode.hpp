#pragma once

#include "../../ASTNode.hpp"
#include "../../GenericTypeParameter.hpp"
#include "../../VisibilityModifier.hpp"
#include <vector>
#include <memory>

namespace ast::nodes::classes
{
    class InterfaceNode : public ASTNode
    {
    private:
        std::string name;
        std::vector<GenericTypeParameter> genericParameters;
        std::vector<std::unique_ptr<ASTNode>> methods; // Method signatures only
        std::vector<std::string> extendsInterfaces; // Parent interfaces
        bool finalInterface; // NEW: Final modifier to prevent extension
        VisibilityModifier visibility; // NEW: Top-level visibility for imports
    public:
        explicit InterfaceNode(const std::string& interfaceName,
                               const std::vector<GenericTypeParameter>& generics = {},
                               const SourceLocation& loc = SourceLocation());


        void addMethod(std::unique_ptr<ASTNode> method);

        const std::string& getName() const;
        const std::vector<GenericTypeParameter>& getGenericParameters() const;
        bool isGeneric() const;

        // Interface inheritance methods
        void addExtendedInterface(const std::string& interfaceName);
        const std::vector<std::string>& getExtendedInterfaces() const;
        bool extendsInterface(const std::string& interfaceName) const;
        std::string getFullInterfaceName() const; // Include generics like "Container<T>"

        // Method access
        const std::vector<std::unique_ptr<ASTNode>>& getMethods() const;

        // NEW: Final modifier methods
        bool isFinal() const;
        void setFinal(bool isFinal);

        // NEW: Visibility modifier methods (for import/export system)
        VisibilityModifier getVisibility() const;
        void setVisibility(VisibilityModifier vis);
        bool isPublic() const;
        bool isPrivate() const;

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
