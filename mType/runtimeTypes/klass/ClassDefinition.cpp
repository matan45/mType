#include "ClassDefinition.hpp"
#include "InterfaceRegistry.hpp"
#include "InterfaceDefinition.hpp"
#include "../../vm/MethodSignature.hpp"
#include "../../types/TypeSubstitutionService.hpp"
#include "../../value/ValueShim.hpp"
#include <algorithm>
#include <set>

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

    ConstructorDefinition* ClassDefinition::findConstructorByTypes(std::span<const value::Value> args) const
    {
        size_t argCount = args.size();

        // Try each constructor with matching parameter count
        for (const auto& constructor : constructors) {
            if (constructor->getParameterCount() != argCount) {
                continue;
            }

            // Check if all argument types match parameter types
            const auto& params = constructor->getParametersWithTypes();
            bool allMatch = true;

            for (size_t i = 0; i < argCount; ++i) {
                const auto& arg = args[i];
                const auto& paramType = params[i].second;

                // Get the runtime type of the argument
                value::ValueType argType;
                if (value::isInt(arg)) {
                    argType = value::ValueType::INT;
                } else if (value::isFloat(arg)) {
                    argType = value::ValueType::FLOAT;
                } else if (value::isBool(arg)) {
                    argType = value::ValueType::BOOL;
                } else if (value::isString(arg) || value::isInternedString(arg)) {
                    argType = value::ValueType::STRING;
                } else if (value::isVoid(arg) || value::isNullType(arg)) {
                    argType = value::ValueType::VOID; // null
                } else {
                    argType = value::ValueType::OBJECT;
                }

                // Check if type matches
                if (argType != paramType.basicType) {
                    // Special case: allow auto-boxing primitives to objects
                    if (paramType.basicType == value::ValueType::OBJECT && paramType.className.has_value()) {
                        const std::string& expectedClass = paramType.className.value();
                        if ((expectedClass == "Int" && argType == value::ValueType::INT) ||
                            (expectedClass == "Float" && argType == value::ValueType::FLOAT) ||
                            (expectedClass == "Bool" && argType == value::ValueType::BOOL) ||
                            (expectedClass == "String" && argType == value::ValueType::STRING)) {
                            // Auto-boxing allowed
                            continue;
                        }
                    }
                    allMatch = false;
                    break;
                }
            }

            if (allMatch) {
                return constructor.get();
            }
        }

        // Fallback to old behavior if no type match found
        return findConstructor(argCount);
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

    const std::unordered_map<std::string, std::vector<std::shared_ptr<MethodDefinition>>>& ClassDefinition::getInstanceMethods() const
    {
        return instanceMethods;
    }

    const std::unordered_map<std::string, std::shared_ptr<FieldDefinition>>& ClassDefinition::getStaticFields() const
    {
        return staticFields;
    }

    const std::unordered_map<std::string, std::vector<std::shared_ptr<MethodDefinition>>>& ClassDefinition::getStaticMethods() const
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
        // NEW: Support overloading - append to vector of overloads
        instanceMethods[name].push_back(method);
    }

    void ClassDefinition::addStaticField(const std::string& name, std::shared_ptr<FieldDefinition> field)
    {
        staticFields[name] = field;
    }

    void ClassDefinition::addStaticMethod(const std::string& name, std::shared_ptr<MethodDefinition> method)
    {
        // NEW: Support overloading - append to vector of overloads
        staticMethods[name].push_back(method);
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

    size_t ClassDefinition::getTotalFieldCount() const
    {
        // PERFORMANCE: Return cached value if available
        if (totalFieldCountCached) {
            return cachedTotalFieldCount;
        }

        // Count this class's fields
        size_t count = instanceFields.size();

        // Add parent class fields
        auto parent = parentClass.lock();
        if (parent) {
            count += parent->getTotalFieldCount();
        }

        // Cache the result
        cachedTotalFieldCount = count;
        totalFieldCountCached = true;
        return count;
    }

    void ClassDefinition::buildFieldIndexMap() const
    {
        if (fieldIndexMapBuilt) return;

        fieldIndexMap.clear();
        fieldIndexToName.clear();
        ownFieldIndexMap.clear();

        // Inherit parent class slots so ancestor fields keep their original
        // indices. When this class shadows an ancestor field, the ancestor's
        // value is still reachable via the ancestor's getOwnFieldIndex.
        auto parent = parentClass.lock();
        if (parent)
        {
            parent->buildFieldIndexMap();
            fieldIndexMap = parent->fieldIndexMap;
            fieldIndexToName = parent->fieldIndexToName;
        }

        // MYT-212: append a fresh slot for every field declared in THIS class,
        // even when the name shadows an ancestor. The shadowed inherited entry
        // remains in fieldIndexToName at its old position; fieldIndexMap[name]
        // is overwritten so getFieldIndex(name) — used by the dynamic-dispatch
        // GET_FIELD path — returns the most-derived slot.
        for (const auto& [name, field] : instanceFields)
        {
            size_t index = fieldIndexToName.size();
            fieldIndexToName.push_back(name);
            fieldIndexMap[name] = index;
            ownFieldIndexMap[name] = index;
        }

        fieldIndexMapBuilt = true;
    }

    size_t ClassDefinition::getFieldIndex(const std::string& fieldName) const
    {
        buildFieldIndexMap();
        auto it = fieldIndexMap.find(fieldName);
        if (it != fieldIndexMap.end())
        {
            return it->second;
        }
        return SIZE_MAX;
    }

    size_t ClassDefinition::getOwnFieldIndex(const std::string& fieldName) const
    {
        buildFieldIndexMap();
        auto it = ownFieldIndexMap.find(fieldName);
        if (it != ownFieldIndexMap.end())
        {
            return it->second;
        }
        return SIZE_MAX;
    }

    size_t ClassDefinition::getIndexedFieldCount() const
    {
        buildFieldIndexMap();
        return fieldIndexToName.size();
    }

    const std::unordered_map<std::string, size_t>& ClassDefinition::getFieldIndexMap() const
    {
        buildFieldIndexMap();
        return fieldIndexMap;
    }

    const std::vector<std::string>& ClassDefinition::getFieldIndexToName() const
    {
        buildFieldIndexMap();
        return fieldIndexToName;
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
        // Try instance method first (most common case)
        auto instanceMethod = findInstanceMethod(methodName, argCount);
        if (instanceMethod) {
            return instanceMethod;
        }

        // Then try static method
        auto staticMethod = findStaticMethod(methodName, argCount);
        if (staticMethod) {
            return staticMethod;
        }

        return nullptr;
    }

    // NEW: Separate static and instance method lookup implementations
    std::shared_ptr<MethodDefinition> ClassDefinition::getStaticMethod(const std::string& methodName) const
    {
        auto it = staticMethods.find(methodName);
        if (it != staticMethods.end() && !it->second.empty()) {
            return it->second[0];  // Return first overload for backward compatibility
        }
        return nullptr;
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::getInstanceMethod(const std::string& methodName) const
    {
        auto it = instanceMethods.find(methodName);
        if (it != instanceMethods.end() && !it->second.empty()) {
            return it->second[0];  // Return first overload for backward compatibility
        }
        return nullptr;
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findStaticMethod(const std::string& methodName, size_t argCount) const
    {
        auto it = staticMethods.find(methodName);
        if (it != staticMethods.end()) {
            // Search all overloads for matching parameter count
            for (const auto& method : it->second) {
                if (method && method->getParameters().size() == argCount) {
                    return method;
                }
            }
        }
        return nullptr;
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findInstanceMethod(const std::string& methodName, size_t argCount) const
    {
        auto it = instanceMethods.find(methodName);
        if (it != instanceMethods.end()) {
            // Search all overloads for matching parameter count
            for (const auto& method : it->second) {
                if (method) {
                    // For instance methods, skip the first parameter ('this') when counting
                    size_t methodParamCount = method->getParameters().size();
                    if (!method->isStatic() && methodParamCount > 0) {
                        methodParamCount--; // Exclude 'this'
                    }
                    if (methodParamCount == argCount) {
                        return method;
                    }
                }
            }
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

    ClassDefinition::MethodLookupResult ClassDefinition::findInstanceMethodCached(
        const std::string& methodName, size_t argCount) const
    {
        // Build cache key
        std::string cacheKey = methodName + "/" + std::to_string(argCount);

        // Check cache first
        auto cacheIt = instanceMethodCache.find(cacheKey);
        if (cacheIt != instanceMethodCache.end()) {
            return cacheIt->second;
        }

        // Not in cache - do the lookup and find defining class in single traversal
        MethodLookupResult result;

        // Check this class first
        auto method = findInstanceMethod(methodName, argCount);
        if (method) {
            result.method = method;
            result.definingClassName = getName();
            // PERFORMANCE: Pre-build the qualified name for function lookup
            auto signature = vm::MethodSignature::fromMethodDefinition(method.get());
            result.qualifiedName = signature.toMangledName(result.definingClassName, false);
            instanceMethodCache[cacheKey] = result;
            return result;
        }

        // Search parent hierarchy - find both method and defining class
        auto current = parentClass.lock();
        int depth = 0;

        while (current && depth < MAX_INHERITANCE_DEPTH) {
            method = current->findInstanceMethod(methodName, argCount);
            if (method) {
                result.method = method;
                result.definingClassName = current->getName();
                // PERFORMANCE: Pre-build the qualified name for function lookup
                auto signature = vm::MethodSignature::fromMethodDefinition(method.get());
                result.qualifiedName = signature.toMangledName(result.definingClassName, false);
                instanceMethodCache[cacheKey] = result;
                return result;
            }
            current = current->parentClass.lock();
            depth++;
        }

        // Not found - cache the negative result too
        result.method = nullptr;
        result.definingClassName = "";
        result.qualifiedName = "";
        instanceMethodCache[cacheKey] = result;
        return result;
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

                    // findInstanceMethod expects parameter count WITHOUT 'this', so subtract 1
                    // For instance methods, getParameters() includes 'this' as the first parameter
                    size_t argCountWithoutThis = (requiredArgCount > 0) ? requiredArgCount - 1 : 0;

                    // First check in current class with signature matching (name + parameter count)
                    auto implementingMethod = findInstanceMethod(abstractMethod, argCountWithoutThis);
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

                            // Use signature-aware lookup (name + parameter count WITHOUT 'this')
                            auto intermediateMethod = intermediateClass->findInstanceMethod(abstractMethod, argCountWithoutThis);
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

    // NEW: Overload-specific method implementations
    std::vector<std::shared_ptr<MethodDefinition>> ClassDefinition::getAllInstanceMethodOverloads(const std::string& methodName) const
    {
        auto it = instanceMethods.find(methodName);
        if (it != instanceMethods.end()) {
            return it->second;
        }
        return {};
    }

    std::vector<std::shared_ptr<MethodDefinition>> ClassDefinition::getAllStaticMethodOverloads(const std::string& methodName) const
    {
        auto it = staticMethods.find(methodName);
        if (it != staticMethods.end()) {
            return it->second;
        }
        return {};
    }

    std::vector<std::shared_ptr<MethodDefinition>> ClassDefinition::getAllInstanceMethodOverloadsInHierarchy(const std::string& methodName) const
    {
        std::vector<std::shared_ptr<MethodDefinition>> allOverloads;
        std::set<std::string> seenSignatures;  // Track signatures to avoid duplicates from overrides

        // Helper to generate signature for deduplication
        // For instance methods, skip the 'this' parameter (first parameter) for signature comparison
        // This allows proper deduplication when a derived class overrides a base class method
        auto getSignature = [](const std::shared_ptr<MethodDefinition>& method) {
            std::string sig = method->getName() + "(";
            const auto& params = method->getParametersWithTypes();
            // Skip first parameter if it's 'this' (for instance methods)
            size_t startIdx = (!method->isStatic() && !params.empty() && params[0].first == "this") ? 1 : 0;
            for (size_t i = startIdx; i < params.size(); ++i) {
                if (i > startIdx) sig += ",";
                sig += params[i].second.toString();
            }
            sig += ")";
            return sig;
        };

        // Collect overloads from this class
        auto localOverloads = getAllInstanceMethodOverloads(methodName);
        for (const auto& overload : localOverloads) {
            std::string sig = getSignature(overload);
            if (seenSignatures.insert(sig).second) {
                allOverloads.push_back(overload);
            }
        }

        // Collect overloads from parent hierarchy
        // Skip methods that have already been overridden in child classes
        auto parent = getParentClass();
        while (parent) {
            auto parentOverloads = parent->getAllInstanceMethodOverloads(methodName);
            for (const auto& overload : parentOverloads) {
                std::string sig = getSignature(overload);
                if (seenSignatures.insert(sig).second) {
                    // Only add if we haven't seen this signature before (not overridden)
                    allOverloads.push_back(overload);
                }
            }
            parent = parent->getParentClass();
        }

        return allOverloads;
    }

    std::vector<std::shared_ptr<MethodDefinition>> ClassDefinition::getAllStaticMethodOverloadsInHierarchy(const std::string& methodName) const
    {
        std::vector<std::shared_ptr<MethodDefinition>> allOverloads;
        std::set<std::string> seenSignatures;  // Track signatures to avoid duplicates

        // Helper to generate signature for deduplication
        // For instance methods, skip the 'this' parameter (first parameter) for signature comparison
        // This allows proper deduplication when a derived class overrides a base class method
        auto getSignature = [](const std::shared_ptr<MethodDefinition>& method) {
            std::string sig = method->getName() + "(";
            const auto& params = method->getParametersWithTypes();
            // Skip first parameter if it's 'this' (for instance methods)
            size_t startIdx = (!method->isStatic() && !params.empty() && params[0].first == "this") ? 1 : 0;
            for (size_t i = startIdx; i < params.size(); ++i) {
                if (i > startIdx) sig += ",";
                sig += params[i].second.toString();
            }
            sig += ")";
            return sig;
        };

        // Collect overloads from this class
        auto localOverloads = getAllStaticMethodOverloads(methodName);
        for (const auto& overload : localOverloads) {
            std::string sig = getSignature(overload);
            if (seenSignatures.insert(sig).second) {
                allOverloads.push_back(overload);
            }
        }

        // Collect overloads from parent hierarchy
        auto parent = getParentClass();
        while (parent) {
            auto parentOverloads = parent->getAllStaticMethodOverloads(methodName);
            for (const auto& overload : parentOverloads) {
                std::string sig = getSignature(overload);
                if (seenSignatures.insert(sig).second) {
                    // Only add if we haven't seen this signature before
                    allOverloads.push_back(overload);
                }
            }
            parent = parent->getParentClass();
        }

        return allOverloads;
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findInstanceMethodBySignature(
        const std::string& methodName,
        const std::vector<std::pair<std::string, value::ParameterType>>& parameters) const
    {
        auto it = instanceMethods.find(methodName);
        if (it == instanceMethods.end()) {
            return nullptr;
        }

        // Search all overloads for exact signature match
        for (const auto& method : it->second) {
            if (!method) continue;

            const auto& methodParams = method->getParametersWithTypes();

            // IMPORTANT: Exclude 'this' parameter when comparing signatures
            // Instance methods have an implicit 'this' parameter that should not affect signature matching
            std::vector<std::pair<std::string, value::ParameterType>> methodParamsWithoutThis;
            for (const auto& param : methodParams) {
                if (param.first != "this") {
                    methodParamsWithoutThis.push_back(param);
                }
            }

            if (methodParamsWithoutThis.size() != parameters.size()) {
                continue;
            }

            // Check if all parameter types match
            bool allMatch = true;
            for (size_t i = 0; i < parameters.size(); ++i) {
                if (!(methodParamsWithoutThis[i].second == parameters[i].second)) {
                    allMatch = false;
                    break;
                }
            }

            if (allMatch) {
                return method;
            }
        }

        return nullptr;
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findStaticMethodBySignature(
        const std::string& methodName,
        const std::vector<std::pair<std::string, value::ParameterType>>& parameters) const
    {
        auto it = staticMethods.find(methodName);
        if (it == staticMethods.end()) {
            return nullptr;
        }

        // Search all overloads for exact signature match
        for (const auto& method : it->second) {
            if (!method) continue;

            const auto& methodParams = method->getParametersWithTypes();
            if (methodParams.size() != parameters.size()) {
                continue;
            }

            // Check if all parameter types match
            bool allMatch = true;
            for (size_t i = 0; i < parameters.size(); ++i) {
                if (!(methodParams[i].second == parameters[i].second)) {
                    allMatch = false;
                    break;
                }
            }

            if (allMatch) {
                return method;
            }
        }

        return nullptr;
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findInstanceMethodBySignatureInHierarchy(
        const std::string& methodName,
        const std::vector<std::pair<std::string, value::ParameterType>>& parameters) const
    {
        // First, check in this class (instance methods only)
        auto method = findInstanceMethodBySignature(methodName, parameters);
        if (method) {
            return method;
        }

        // Then check in parent class hierarchy
        return traverseHierarchyForMethod([&](const ClassDefinition* classDef) {
            return classDef->findInstanceMethodBySignature(methodName, parameters);
        });
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findStaticMethodBySignatureInHierarchy(
        const std::string& methodName,
        const std::vector<std::pair<std::string, value::ParameterType>>& parameters) const
    {
        // First, check in this class (static methods only)
        auto method = findStaticMethodBySignature(methodName, parameters);
        if (method) {
            return method;
        }

        // Then check in parent class hierarchy
        return traverseHierarchyForMethod([&](const ClassDefinition* classDef) {
            return classDef->findStaticMethodBySignature(methodName, parameters);
        });
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findInstanceMethodByTypeSignature(
        const std::string& methodName,
        const std::string& typeSignature) const
    {
        auto it = instanceMethods.find(methodName);
        if (it == instanceMethods.end()) {
            return nullptr;
        }

        // Search all overloads for matching type signature
        for (const auto& method : it->second) {
            if (!method) continue;

            // Generate signature for this method and compare
            std::string methodSig;
            const auto& params = method->getParametersWithTypes();
            for (size_t i = 0; i < params.size(); ++i) {
                if (i > 0) methodSig += ",";
                // Use ParameterType's toString() method
                methodSig += params[i].second.toString();
            }

            if (methodSig == typeSignature) {
                return method;
            }
        }

        return nullptr;
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findStaticMethodByTypeSignature(
        const std::string& methodName,
        const std::string& typeSignature) const
    {
        auto it = staticMethods.find(methodName);
        if (it == staticMethods.end()) {
            return nullptr;
        }

        // Search all overloads for matching type signature
        for (const auto& method : it->second) {
            if (!method) continue;

            // Generate signature for this method and compare
            std::string methodSig;
            const auto& params = method->getParametersWithTypes();
            for (size_t i = 0; i < params.size(); ++i) {
                if (i > 0) methodSig += ",";
                // Use ParameterType's toString() method
                methodSig += params[i].second.toString();
            }

            if (methodSig == typeSignature) {
                return method;
            }
        }

        return nullptr;
    }

    // NEW: MethodSignature-based lookups
    std::shared_ptr<MethodDefinition> ClassDefinition::findInstanceMethod(const vm::MethodSignature& signature) const
    {
        // Use the argCount-based method with signature's parameter count
        return findInstanceMethod(signature.getMethodName(), signature.getParameterCount());
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findStaticMethod(const vm::MethodSignature& signature) const
    {
        // Use the argCount-based method with signature's parameter count
        return findStaticMethod(signature.getMethodName(), signature.getParameterCount());
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findInstanceMethodInHierarchy(const vm::MethodSignature& signature) const
    {
        // Use the argCount-based method with signature's parameter count
        return findInstanceMethodInHierarchy(signature.getMethodName(), signature.getParameterCount());
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findStaticMethodInHierarchy(const vm::MethodSignature& signature) const
    {
        // Use the argCount-based method with signature's parameter count
        return findStaticMethodInHierarchy(signature.getMethodName(), signature.getParameterCount());
    }

    // NEW (Phase 4): Resolve a type considering the full inheritance chain
    ::types::UnifiedTypePtr ClassDefinition::resolveTypeInContext(const ::types::UnifiedTypePtr& type) const
    {
        if (!type)
        {
            return nullptr;
        }

        // If no inheritance chain, just use local substitution map
        if (inheritanceSubstitutionChain.empty())
        {
            if (parentTypeSubstitutionMap.empty())
            {
                return type;
            }

            // Convert string-based map to UnifiedType-based map for backward compatibility
            ::types::TypeSubstitutionMap legacyMap;
            for (const auto& [param, concrete] : parentTypeSubstitutionMap)
            {
                legacyMap[param] = ::types::UnifiedType::classType(concrete);
            }

            ::types::TypeSubstitutionService service;
            return service.substitute(type, legacyMap);
        }

        // Use the TypeSubstitutionService to compose the full chain and substitute
        ::types::TypeSubstitutionService service;
        auto composedMap = service.composeChain(inheritanceSubstitutionChain);
        return service.substitute(type, composedMap);
    }
}
