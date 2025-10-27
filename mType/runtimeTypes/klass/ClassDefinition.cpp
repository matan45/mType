#include "ClassDefinition.hpp"
#include "InterfaceRegistry.hpp"
#include "InterfaceDefinition.hpp"
#include <algorithm>

namespace runtimeTypes::klass
{
    // Query methods
    bool ClassDefinition::hasField(const std::string& fieldName) const
    {
        return instanceFields.find(fieldName) != instanceFields.end();
    }

    bool ClassDefinition::hasMethod(const std::string& methodName) const
    {
        return instanceMethods.find(methodName) != instanceMethods.end();
    }

    bool ClassDefinition::hasStaticField(const std::string& fieldName) const
    {
        return staticFields.find(fieldName) != staticFields.end();
    }

    bool ClassDefinition::hasStaticMethod(const std::string& methodName) const
    {
        return staticMethods.find(methodName) != staticMethods.end();
    }

    ConstructorDefinition* ClassDefinition::findConstructor(size_t argCount) const
    {
        for (const auto& constructor : constructors) {
            if (constructor->getParameterCount() == argCount) {
                return constructor.get();
            }
        }
        return nullptr;
    }

    // Getter methods for AST node integration
    const std::string& ClassDefinition::getClassName() const
    {
        return getName(); // From base Definition class
    }

    const std::unordered_map<std::string, std::shared_ptr<FieldDefinition>>& ClassDefinition::getInstanceFields() const
    {
        return instanceFields;
    }

    const std::unordered_map<std::string, std::shared_ptr<MethodDefinition>>& ClassDefinition::getInstanceMethods() const
    {
        return instanceMethods;
    }

    const std::unordered_map<std::string, std::shared_ptr<FieldDefinition>>& ClassDefinition::getStaticFields() const
    {
        return staticFields;
    }

    const std::unordered_map<std::string, std::shared_ptr<MethodDefinition>>& ClassDefinition::getStaticMethods() const
    {
        return staticMethods;
    }

    const std::vector<std::shared_ptr<ConstructorDefinition>>& ClassDefinition::getConstructors() const
    {
        return constructors;
    }

    // Setter/adder methods for AST node integration
    void ClassDefinition::addInstanceField(const std::string& name, std::shared_ptr<FieldDefinition> field)
    {
        instanceFields[name] = field;
    }

    void ClassDefinition::addInstanceMethod(const std::string& name, std::shared_ptr<MethodDefinition> method)
    {
        instanceMethods[name] = method;
    }

    void ClassDefinition::addStaticField(const std::string& name, std::shared_ptr<FieldDefinition> field)
    {
        staticFields[name] = field;
    }

    void ClassDefinition::addStaticMethod(const std::string& name, std::shared_ptr<MethodDefinition> method)
    {
        staticMethods[name] = method;
    }

    void ClassDefinition::addConstructor(std::shared_ptr<ConstructorDefinition> constructor)
    {
        constructors.push_back(constructor);
    }

    // Utility methods
    size_t ClassDefinition::getInstanceFieldCount() const
    {
        return instanceFields.size();
    }

    size_t ClassDefinition::getInstanceMethodCount() const
    {
        return instanceMethods.size();
    }

    size_t ClassDefinition::getStaticFieldCount() const
    {
        return staticFields.size();
    }

    size_t ClassDefinition::getStaticMethodCount() const
    {
        return staticMethods.size();
    }

    size_t ClassDefinition::getConstructorCount() const
    {
        return constructors.size();
    }

    // Additional compatibility methods implementation
    void ClassDefinition::addField(std::shared_ptr<FieldDefinition> field)
    {
        if (field->isStatic()) {
            addStaticField(field->getName(), field);
        } else {
            addInstanceField(field->getName(), field);
        }
    }

    void ClassDefinition::addMethod(std::shared_ptr<MethodDefinition> method)
    {
        if (method->isStatic()) {
            addStaticMethod(method->getName(), method);
        } else {
            addInstanceMethod(method->getName(), method);
        }
    }

    std::shared_ptr<FieldDefinition> ClassDefinition::getField(const std::string& fieldName) const
    {
        auto it = instanceFields.find(fieldName);
        if (it != instanceFields.end()) {
            return it->second;
        }

        auto staticIt = staticFields.find(fieldName);
        if (staticIt != staticFields.end()) {
            return staticIt->second;
        }

        return nullptr;
    }

    std::shared_ptr<FieldDefinition> ClassDefinition::getFieldInHierarchy(const std::string& fieldName) const
    {
        // First, check in this class
        auto field = getField(fieldName);
        if (field) {
            return field;
        }

        // Then check in parent class hierarchy
        auto current = parentClass.lock();
        int depth = 0;

        while (current && depth < MAX_INHERITANCE_DEPTH) {
            field = current->getField(fieldName);
            if (field) {
                return field;
            }
            current = current->parentClass.lock();
            depth++;
        }

        return nullptr;
    }

    std::shared_ptr<ClassDefinition> ClassDefinition::getFieldOwnerInHierarchy(
        const std::string& fieldName,
        std::shared_ptr<ClassDefinition> self) const
    {
        // First, check if this class owns the field
        auto field = getField(fieldName);
        if (field) {
            return self; // Return the shared_ptr to this class
        }

        // Then check in parent class hierarchy
        auto current = parentClass.lock();
        int depth = 0;

        while (current && depth < MAX_INHERITANCE_DEPTH) {
            field = current->getField(fieldName);
            if (field) {
                return current; // Return the parent class that owns the field
            }
            current = current->parentClass.lock();
            depth++;
        }

        return nullptr; // Field not found in hierarchy
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::getMethod(const std::string& methodName) const
    {
        auto method = getInstanceMethod(methodName);
        if (method) {
            return method;
        }
        return getStaticMethod(methodName);
    }

    std::shared_ptr<ConstructorDefinition> ClassDefinition::getConstructor() const
    {
        return constructors.empty() ? nullptr : constructors[0];
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findMethod(const std::string& methodName, size_t argCount) const
    {
        auto method = getMethod(methodName);
        if (method && method->getParameters().size() == argCount) {
            return method;
        }
        return nullptr;
    }

    // NEW: Separate static and instance method lookup implementations
    std::shared_ptr<MethodDefinition> ClassDefinition::getStaticMethod(const std::string& methodName) const
    {
        auto it = staticMethods.find(methodName);
        if (it != staticMethods.end()) {
            return it->second;
        }
        return nullptr;
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::getInstanceMethod(const std::string& methodName) const
    {
        auto it = instanceMethods.find(methodName);
        if (it != instanceMethods.end()) {
            return it->second;
        }
        return nullptr;
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findStaticMethod(const std::string& methodName, size_t argCount) const
    {
        auto method = getStaticMethod(methodName);
        if (method && method->getParameters().size() == argCount) {
            return method;
        }
        return nullptr;
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findInstanceMethod(const std::string& methodName, size_t argCount) const
    {
        auto method = getInstanceMethod(methodName);
        if (method && method->getParameters().size() == argCount) {
            return method;
        }
        return nullptr;
    }

    // NEW: Generic-related method implementations
    std::string ClassDefinition::getGenericClassName() const
    {
        if (!isGeneric()) {
            return getName();
        }

        std::string result = getName() + "<";
        for (size_t i = 0; i < genericParameters.size(); ++i) {
            if (i > 0) result += ", ";
            result += genericParameters[i].name;
        }
        result += ">";
        return result;
    }

    bool ClassDefinition::hasGenericParameter(const std::string& paramName) const
    {
        for (const auto& param : genericParameters) {
            if (param.name == paramName) {
                return true;
            }
        }
        return false;
    }

    bool ClassDefinition::implementsInterface(const std::string& interfaceName) const
    {
        // Direct interface checking only - for transitive checks, use the registry-based version
        for (const auto& implementedInterface : implementedInterfaces) {
            std::string baseImplementedName = extractBaseTypeName(implementedInterface);

            if (baseImplementedName == interfaceName) {
                return true;
            }
        }

        // Cannot perform transitive interface resolution without InterfaceRegistry
        return false;
    }

    bool ClassDefinition::implementsInterface(const std::string& interfaceName, std::shared_ptr<InterfaceRegistry> registry) const
    {
        // Normalize the expected interface name (remove spaces after commas in generics)
        std::string normalizedExpected = normalizeGenericTypeName(interfaceName);

        // Check direct interfaces first
        for (const auto& implementedInterface : implementedInterfaces) {
            // Normalize the implemented interface name for comparison
            std::string normalizedImplemented = normalizeGenericTypeName(implementedInterface);

            // Compare full names (including generic type arguments)
            if (normalizedImplemented == normalizedExpected) {
                return true;
            }

            // Also check if base names match (for backwards compatibility with non-generic interfaces)
            std::string baseImplementedName = extractBaseTypeName(implementedInterface);
            std::string baseExpectedName = extractBaseTypeName(interfaceName);
            if (baseImplementedName == baseExpectedName) {
                return true;
            }
        }

        // Check transitive interface inheritance with depth protection and registry access
        std::unordered_set<std::string> visited;
        return implementsInterfaceTransitive(interfaceName, visited, 0, registry);
    }

    std::string ClassDefinition::extractBaseTypeName(const std::string& typeName)
    {
        size_t anglePos = typeName.find('<');
        return (anglePos != std::string::npos) ? typeName.substr(0, anglePos) : typeName;
    }

    std::string ClassDefinition::normalizeGenericTypeName(const std::string& typeName)
    {
        // Remove spaces after commas in generic type arguments
        // E.g., "Function<Int, String>" -> "Function<Int,String>"
        std::string normalized;
        normalized.reserve(typeName.size());

        for (size_t i = 0; i < typeName.size(); ++i) {
            char c = typeName[i];

            // Skip spaces that come after commas (when inside angle brackets)
            if (c == ' ' && i > 0 && typeName[i - 1] == ',') {
                continue;
            }

            normalized += c;
        }

        return normalized;
    }

    bool ClassDefinition::checkDirectInterfaceMatch(const std::string& interfaceName,
                                                     const std::string& implementedInterface,
                                                     std::unordered_set<std::string>& visited,
                                                     std::shared_ptr<InterfaceRegistry> registry) const
    {
        std::string baseImplementedName = extractBaseTypeName(implementedInterface);

        // Avoid infinite recursion (circular inheritance)
        if (visited.find(baseImplementedName) != visited.end()) {
            return false;
        }

        visited.insert(baseImplementedName);

        // Look up the interface definition in the registry
        auto interfaceDef = registry->findInterface(baseImplementedName);
        if (interfaceDef) {
            // Check what interfaces this interface extends
            const auto& extendedInterfaces = interfaceDef->getExtendedInterfaces();
            for (const auto& extendedInterface : extendedInterfaces) {
                std::string baseExtendedName = extractBaseTypeName(extendedInterface);

                // Check if this extended interface matches what we're looking for
                if (baseExtendedName == interfaceName) {
                    visited.erase(baseImplementedName);
                    return true;
                }

                // Recursively check if the extended interface implements our target
                auto extendedInterfaceDef = registry->findInterface(baseExtendedName);
                if (extendedInterfaceDef) {
                    const auto& furtherExtended = extendedInterfaceDef->getExtendedInterfaces();
                    for (const auto& furtherInterface : furtherExtended) {
                        std::string furtherBaseName = extractBaseTypeName(furtherInterface);

                        if (furtherBaseName == interfaceName) {
                            visited.erase(baseImplementedName);
                            return true;
                        }
                    }
                }
            }
        }

        visited.erase(baseImplementedName);
        return false;
    }

    bool ClassDefinition::implementsInterfaceTransitive(const std::string& interfaceName,
                                                       std::unordered_set<std::string>& visited,
                                                       int depth,
                                                       std::shared_ptr<InterfaceRegistry> registry) const
    {
        // Depth protection - prevent stack overflow attacks
        if (depth > MAX_INTERFACE_DEPTH) {
            return false;
        }

        if (!registry) {
            return false;
        }

        // Check all directly implemented interfaces for transitive inheritance
        for (const auto& implementedInterface : implementedInterfaces) {
            if (checkDirectInterfaceMatch(interfaceName, implementedInterface, visited, registry)) {
                return true;
            }
        }

        return false;
    }

    bool ClassDefinition::isSubclassOf(const std::string& className) const
    {
        // Check direct parent
        if (parentClassName == className) {
            return true;
        }

        // Traverse inheritance chain with depth protection
        auto current = parentClass.lock();
        int depth = 0;

        while (current && depth < MAX_INHERITANCE_DEPTH) {
            if (current->getName() == className) {
                return true;
            }
            current = current->parentClass.lock();
            depth++;
        }

        return false;
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findMethodInHierarchy(const std::string& methodName, size_t argCount) const
    {
        // First, check in this class
        auto method = findMethod(methodName, argCount);
        if (method) {
            return method;
        }

        // Then check in parent class hierarchy
        return traverseHierarchyForMethod([&](const ClassDefinition* classDef) {
            return classDef->findMethod(methodName, argCount);
        });
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findInstanceMethodInHierarchy(const std::string& methodName, size_t argCount) const
    {
        // First, check in this class (instance methods only)
        auto method = findInstanceMethod(methodName, argCount);
        if (method) {
            return method;
        }

        // Then check in parent class hierarchy
        return traverseHierarchyForMethod([&](const ClassDefinition* classDef) {
            return classDef->findInstanceMethod(methodName, argCount);
        });
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findStaticMethodInHierarchy(const std::string& methodName, size_t argCount) const
    {
        // First, check in this class (static methods only)
        auto method = findStaticMethod(methodName, argCount);
        if (method) {
            return method;
        }

        // Then check in parent class hierarchy
        return traverseHierarchyForMethod([&](const ClassDefinition* classDef) {
            return classDef->findStaticMethod(methodName, argCount);
        });
    }

    std::vector<std::shared_ptr<ClassDefinition>> ClassDefinition::getInheritanceChain() const
    {
        std::vector<std::shared_ptr<ClassDefinition>> chain;

        auto current = parentClass.lock();
        int depth = 0;

        while (current && depth < MAX_INHERITANCE_DEPTH) {
            chain.push_back(current);
            current = current->parentClass.lock();
            depth++;
        }

        return chain;
    }

    std::vector<std::string> ClassDefinition::getUnimplementedAbstractMethods() const
    {
        std::vector<std::string> unimplemented;

        // Collect abstract methods from this class
        for (const auto& methodName : abstractMethods) {
            unimplemented.push_back(methodName);
        }

        // Collect abstract methods from parent classes in the inheritance chain
        auto chain = getInheritanceChain();
        for (const auto& parentClass : chain) {
            if (parentClass && parentClass->isAbstract()) {
                for (const auto& abstractMethod : parentClass->getAbstractMethods()) {
                    // Check if this abstract method is implemented anywhere in the class hierarchy
                    // (from current class up to but not including the abstract class that declared it)
                    bool isImplemented = false;

                    // Get the abstract method definition to extract signature information
                    auto abstractMethodDef = parentClass->getInstanceMethod(abstractMethod);
                    if (!abstractMethodDef) {
                        continue; // Skip if definition not found (shouldn't happen)
                    }

                    size_t requiredArgCount = abstractMethodDef->getParameters().size();

                    // First check in current class with signature matching (name + parameter count)
                    auto implementingMethod = findInstanceMethod(abstractMethod, requiredArgCount);
                    if (implementingMethod && !implementingMethod->isAbstract()) {
                        isImplemented = true;
                    }

                    // If not implemented in current class, check intermediate classes in the chain
                    if (!isImplemented) {
                        for (const auto& intermediateClass : chain) {
                            // Stop when we reach the class that declared the abstract method
                            if (intermediateClass == parentClass) {
                                break;
                            }

                            // Use signature-aware lookup (name + parameter count)
                            auto intermediateMethod = intermediateClass->findInstanceMethod(abstractMethod, requiredArgCount);
                            if (intermediateMethod && !intermediateMethod->isAbstract()) {
                                isImplemented = true;
                                break;
                            }
                        }
                    }

                    // If not implemented, add to unimplemented list (avoid duplicates)
                    if (!isImplemented && std::find(unimplemented.begin(), unimplemented.end(), abstractMethod) == unimplemented.end()) {
                        unimplemented.push_back(abstractMethod);
                    }
                }
            }
        }

        return unimplemented;
    }

    bool ClassDefinition::hasAllAbstractMethodsImplemented() const
    {
        // If this class is abstract, it doesn't need to implement all abstract methods
        if (abstractClass) {
            return true;
        }

        // Check if there are any unimplemented abstract methods
        auto unimplemented = getUnimplementedAbstractMethods();
        return unimplemented.empty();
    }

    bool ClassDefinition::hasAnnotation(const std::string& annotationName) const
    {
        for (const auto& annotation : annotations)
        {
            if (annotation->getName() == annotationName)
            {
                return true;
            }
        }
        return false;
    }

    std::shared_ptr<ast::nodes::annotations::AnnotationNode> ClassDefinition::getAnnotation(const std::string& annotationName) const
    {
        for (const auto& annotation : annotations)
        {
            if (annotation->getName() == annotationName)
            {
                return annotation;
            }
        }
        return nullptr;
    }
}
