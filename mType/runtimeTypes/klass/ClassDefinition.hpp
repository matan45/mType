#pragma once
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <vector>

#include "ConstructorDefinition.hpp"
#include "FieldDefinition.hpp"
#include "MethodDefinition.hpp"
#include "../Definition.hpp"
#include "../../ast/GenericTypeParameter.hpp"
#include "../../ast/nodes/annotations/AnnotationNode.hpp"

namespace vm
{
    class MethodSignature; // Forward declaration
}

namespace runtimeTypes::klass
{
    class InterfaceRegistry; // Forward declaration
}

namespace runtimeTypes::klass
{
    class ClassDefinition : public Definition
    {
    private:
        // Instance members
        std::unordered_map<std::string, std::shared_ptr<FieldDefinition>> instanceFields;
        // NEW: Support multiple overloads per method name
        std::unordered_map<std::string, std::vector<std::shared_ptr<MethodDefinition>>> instanceMethods;

        // Static members
        std::unordered_map<std::string, std::shared_ptr<FieldDefinition>> staticFields;
        // NEW: Support multiple overloads per method name
        std::unordered_map<std::string, std::vector<std::shared_ptr<MethodDefinition>>> staticMethods;

        // Constructors and destructor
        std::vector<std::shared_ptr<ConstructorDefinition>> constructors;

        // NEW: Generic support
        std::vector<ast::GenericTypeParameter> genericParameters;
        bool isGenericClass;

        // NEW: Interface support
        std::vector<std::string> implementedInterfaces;

        // NEW: Inheritance support
        std::string parentClassName;
        std::weak_ptr<ClassDefinition> parentClass;

        // NEW: Generic type substitution for parent class
        // Maps parent's generic type parameters to concrete types
        // E.g., when StringProcessor extends Processor<String>, maps: T → String
        std::unordered_map<std::string, std::string> parentTypeSubstitutionMap;

        // NEW: Final modifier to prevent inheritance
        bool finalClass;

        // NEW: Abstract modifier for abstract classes
        bool abstractClass;
        std::unordered_set<std::string> abstractMethods; // Track which methods are abstract

        // NEW: Annotations for this class
        std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>> annotations;

    public:
        explicit ClassDefinition(const std::string& n)
            : Definition(n), isGenericClass(false), finalClass(false), abstractClass(false)
        {
        }

        // NEW: Constructor with generic parameters
        explicit ClassDefinition(const std::string& n, const std::vector<ast::GenericTypeParameter>& generics)
            : Definition(n), genericParameters(generics), isGenericClass(!generics.empty()), finalClass(false),
              abstractClass(false)
        {
        }

        // Query methods
        bool hasField(const std::string& fieldName) const;
        bool hasMethod(const std::string& methodName) const;
        bool hasStaticField(const std::string& fieldName) const;
        bool hasStaticMethod(const std::string& methodName) const;
        ConstructorDefinition* findConstructor(size_t argCount) const;
        ConstructorDefinition* findConstructorByTypes(const std::vector<value::Value>& args) const;

        // Getter methods for AST node integration
        const std::string& getClassName() const;
        const std::unordered_map<std::string, std::shared_ptr<FieldDefinition>>& getInstanceFields() const;
        // NEW: Returns map of method name -> vector of overloads
        const std::unordered_map<std::string, std::vector<std::shared_ptr<MethodDefinition>>>&
        getInstanceMethods() const;
        const std::unordered_map<std::string, std::shared_ptr<FieldDefinition>>& getStaticFields() const;
        // NEW: Returns map of method name -> vector of overloads
        const std::unordered_map<std::string, std::vector<std::shared_ptr<MethodDefinition>>>& getStaticMethods() const;
        const std::vector<std::shared_ptr<ConstructorDefinition>>& getConstructors() const;

        // Setter/adder methods for AST node integration
        void addInstanceField(const std::string& name, std::shared_ptr<FieldDefinition> field);
        void addInstanceMethod(const std::string& name, std::shared_ptr<MethodDefinition> method);
        void addStaticField(const std::string& name, std::shared_ptr<FieldDefinition> field);
        void addStaticMethod(const std::string& name, std::shared_ptr<MethodDefinition> method);
        void addConstructor(std::shared_ptr<ConstructorDefinition> constructor);

        // Utility methods
        size_t getInstanceFieldCount() const;
        size_t getInstanceMethodCount() const;
        size_t getStaticFieldCount() const;
        size_t getStaticMethodCount() const;
        size_t getConstructorCount() const;

        // Additional compatibility methods
        void addField(std::shared_ptr<FieldDefinition> field);
        void addMethod(std::shared_ptr<MethodDefinition> method);
        std::shared_ptr<FieldDefinition> getField(const std::string& fieldName) const;
        std::shared_ptr<MethodDefinition> getMethod(const std::string& methodName) const;
        std::shared_ptr<ConstructorDefinition> getConstructor() const;
        std::shared_ptr<MethodDefinition> findMethod(const std::string& methodName, size_t argCount) const;

        // NEW: Separate static and instance method lookup
        std::shared_ptr<MethodDefinition> getStaticMethod(const std::string& methodName) const;
        std::shared_ptr<MethodDefinition> getInstanceMethod(const std::string& methodName) const;
        std::shared_ptr<MethodDefinition> findStaticMethod(const std::string& methodName, size_t argCount) const;
        std::shared_ptr<MethodDefinition> findInstanceMethod(const std::string& methodName, size_t argCount) const;

        // NEW: Overload-specific methods
        // Get all overloads for a method name
        std::vector<std::shared_ptr<MethodDefinition>> getAllInstanceMethodOverloads(
            const std::string& methodName) const;
        std::vector<std::shared_ptr<MethodDefinition>> getAllStaticMethodOverloads(const std::string& methodName) const;

        // Get all overloads including inherited methods from parent classes
        std::vector<std::shared_ptr<MethodDefinition>> getAllInstanceMethodOverloadsInHierarchy(
            const std::string& methodName) const;
        std::vector<std::shared_ptr<MethodDefinition>> getAllStaticMethodOverloadsInHierarchy(
            const std::string& methodName) const;

        // Find method by signature (with type parameters)
        std::shared_ptr<MethodDefinition> findInstanceMethodBySignature(
            const std::string& methodName,
            const std::vector<std::pair<std::string, value::ParameterType>>& parameters) const;
        std::shared_ptr<MethodDefinition> findStaticMethodBySignature(
            const std::string& methodName,
            const std::vector<std::pair<std::string, value::ParameterType>>& parameters) const;

        // Find method by signature in hierarchy (searches parent classes)
        std::shared_ptr<MethodDefinition> findInstanceMethodBySignatureInHierarchy(
            const std::string& methodName,
            const std::vector<std::pair<std::string, value::ParameterType>>& parameters) const;
        std::shared_ptr<MethodDefinition> findStaticMethodBySignatureInHierarchy(
            const std::string& methodName,
            const std::vector<std::pair<std::string, value::ParameterType>>& parameters) const;

        // Find method by type signature string
        std::shared_ptr<MethodDefinition> findInstanceMethodByTypeSignature(
            const std::string& methodName,
            const std::string& typeSignature) const;
        std::shared_ptr<MethodDefinition> findStaticMethodByTypeSignature(
            const std::string& methodName,
            const std::string& typeSignature) const;

        // NEW: Generic-related methods
        bool isGeneric() const { return isGenericClass; }
        const std::vector<ast::GenericTypeParameter>& getGenericParameters() const { return genericParameters; }

        void setGenericParameters(const std::vector<ast::GenericTypeParameter>& params)
        {
            genericParameters = params;
            isGenericClass = !params.empty();
        }

        size_t getGenericParameterCount() const { return genericParameters.size(); }
        std::string getBaseName() const { return getName(); }

        // Get full generic class name like "Box<T>"
        std::string getGenericClassName() const;

        // Check if a type parameter exists
        bool hasGenericParameter(const std::string& paramName) const;

        // NEW: Interface-related methods
        const std::vector<std::string>& getImplementedInterfaces() const { return implementedInterfaces; }

        void setImplementedInterfaces(const std::vector<std::string>& interfaces)
        {
            implementedInterfaces = interfaces;
        }

        void addImplementedInterface(const std::string& interfaceName)
        {
            implementedInterfaces.push_back(interfaceName);
        }

        // Direct interface checking (does not support transitive interface inheritance)
        bool implementsInterface(const std::string& interfaceName) const;

        // Full interface checking with transitive inheritance support (requires InterfaceRegistry)
        bool implementsInterface(const std::string& interfaceName, std::shared_ptr<InterfaceRegistry> registry) const;

        // NEW: Inheritance-related methods
        bool isSubclassOf(const std::string& className) const;

        // Parent class management
        const std::string& getParentClassName() const { return parentClassName; }
        void setParentClassName(const std::string& parent) { parentClassName = parent; }
        bool hasParentClass() const { return !parentClassName.empty(); }

        std::shared_ptr<ClassDefinition> getParentClass() const { return parentClass.lock(); }

        void setParentClass(std::shared_ptr<ClassDefinition> parent)
        {
            parentClass = parent;
            // Only set parentClassName if it hasn't been explicitly set already
            // This preserves generic type arguments like "BaseContainer<T>"
            if (parent && parentClassName.empty())
            {
                parentClassName = parent->getName();
            }
        }

        // Generic type substitution management
        const std::unordered_map<std::string, std::string>& getParentTypeSubstitutionMap() const
        {
            return parentTypeSubstitutionMap;
        }

        void setParentTypeSubstitutionMap(const std::unordered_map<std::string, std::string>& map)
        {
            parentTypeSubstitutionMap = map;
        }

        void addParentTypeSubstitution(const std::string& genericParam, const std::string& concreteType)
        {
            parentTypeSubstitutionMap[genericParam] = concreteType;
        }

        // Polymorphic method lookup
        std::shared_ptr<MethodDefinition> findMethodInHierarchy(const std::string& methodName, size_t argCount) const;
        std::shared_ptr<MethodDefinition> findInstanceMethodInHierarchy(const std::string& methodName,
                                                                        size_t argCount) const;
        std::shared_ptr<MethodDefinition> findStaticMethodInHierarchy(const std::string& methodName,
                                                                      size_t argCount) const;

        // NEW: MethodSignature-based lookups (recommended over argCount-based methods)
        // These eliminate confusion about implicit 'this' parameter counting
        std::shared_ptr<MethodDefinition> findInstanceMethod(const vm::MethodSignature& signature) const;
        std::shared_ptr<MethodDefinition> findStaticMethod(const vm::MethodSignature& signature) const;
        std::shared_ptr<MethodDefinition> findInstanceMethodInHierarchy(const vm::MethodSignature& signature) const;
        std::shared_ptr<MethodDefinition> findStaticMethodInHierarchy(const vm::MethodSignature& signature) const;

        // Polymorphic field lookup (search in parent classes)
        std::shared_ptr<FieldDefinition> getFieldInHierarchy(const std::string& fieldName) const;

        // NEW: Get field owner class in hierarchy
        // Returns the ClassDefinition that owns the field (important for access control)
        // Pass 'self' as the first parameter when calling from a shared_ptr
        std::shared_ptr<ClassDefinition> getFieldOwnerInHierarchy(
            const std::string& fieldName,
            std::shared_ptr<ClassDefinition> self) const;

        // Inheritance chain traversal
        std::vector<std::shared_ptr<ClassDefinition>> getInheritanceChain() const;

        // NEW: Final modifier methods
        bool isFinal() const { return finalClass; }
        void setFinal(bool isFinal) { finalClass = isFinal; }

        // NEW: Abstract modifier methods
        bool isAbstract() const { return abstractClass; }
        void setAbstract(bool isAbstract) { abstractClass = isAbstract; }

        // Abstract method management
        void addAbstractMethod(const std::string& methodName) { abstractMethods.insert(methodName); }
        void removeAbstractMethod(const std::string& methodName) { abstractMethods.erase(methodName); }
        bool isMethodAbstract(const std::string& methodName) const { return abstractMethods.count(methodName) > 0; }
        const std::unordered_set<std::string>& getAbstractMethods() const { return abstractMethods; }

        // Check if all abstract methods from parent are implemented
        std::vector<std::string> getUnimplementedAbstractMethods() const;
        bool hasAllAbstractMethodsImplemented() const;

        // NEW: Annotation methods
        const std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>& getAnnotations() const
        {
            return annotations;
        }

        void addAnnotation(std::shared_ptr<ast::nodes::annotations::AnnotationNode> annotation)
        {
            annotations.push_back(annotation);
        }

        bool hasAnnotation(const std::string& annotationName) const;
        std::shared_ptr<ast::nodes::annotations::AnnotationNode> getAnnotation(const std::string& annotationName) const;

    private:
        // Depth protection for interface and class inheritance chains
        static constexpr int MAX_INTERFACE_DEPTH = 20;
        static constexpr int MAX_INHERITANCE_DEPTH = 20;

        // Helper method for transitive interface checking with depth protection
        // Requires InterfaceRegistry for complete transitive resolution
        bool implementsInterfaceTransitive(const std::string& interfaceName,
                                           std::unordered_set<std::string>& visited,
                                           int depth,
                                           std::shared_ptr<InterfaceRegistry> registry) const;

        // Helper methods for implementsInterfaceTransitive
        static std::string extractBaseTypeName(const std::string& typeName);
        static std::string normalizeGenericTypeName(const std::string& typeName);
        bool checkDirectInterfaceMatch(const std::string& interfaceName,
                                       const std::string& implementedInterface,
                                       std::unordered_set<std::string>& visited,
                                       std::shared_ptr<InterfaceRegistry> registry) const;

        // Template helper for traversing parent class hierarchy
        template <typename Func>
        std::shared_ptr<MethodDefinition> traverseHierarchyForMethod(Func findFunc) const
        {
            auto current = parentClass.lock();
            int depth = 0;

            while (current && depth < MAX_INHERITANCE_DEPTH)
            {
                auto method = findFunc(current.get());
                if (method)
                {
                    return method;
                }
                current = current->parentClass.lock();
                depth++;
            }

            return nullptr;
        }
    };
}
