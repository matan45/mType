#pragma once
#include "Function.hpp"
#include "../core/Environment.hpp"
#include <map>
#include <set>

namespace mtype::runtime {
    using namespace core;
    
	// Forward declarations
	class MTypeFunction;
	class MTypeConstructor;
	class MTypeBoundMethod;
    
    // Enum class support
    class MTypeEnum {
    private:
        std::string name;
        std::map<std::string, int> values;
        std::map<int, std::string> reverseValues;
        AccessModifier access;
        int nextValue;
        
    public:
        MTypeEnum(const std::string& n, AccessModifier acc = AccessModifier::ACCESS_PUBLIC)
            : name(n), access(acc), nextValue(0) {}
        
        Result<void> addValue(const std::string& constantName, int value = -1) {
            if (values.find(constantName) != values.end()) {
                return Result<void>::err(Error::name(
                    "Enum value '" + constantName + "' already defined in enum '" + name + "'"));
            }
            
            if (value == -1) {
                value = nextValue;
            }
            
            values[constantName] = value;
            reverseValues[value] = constantName;
            nextValue = value + 1;
            
            return Result<void>::ok();
        }
        
        Result<int> getValue(const std::string& constantName) const {
            auto it = values.find(constantName);
            if (it == values.end()) {
                return Result<int>::err(Error::name(
                    "Enum value '" + constantName + "' not found in enum '" + name + "'"));
            }
            return Result<int>::ok(it->second);
        }
        
        Result<std::string> getName(int value) const {
            auto it = reverseValues.find(value);
            if (it == reverseValues.end()) {
                return Result<std::string>::err(Error::name(
                    "No enum value for integer " + std::to_string(value) + " in enum '" + name + "'"));
            }
            return Result<std::string>::ok(it->second);
        }
        
        bool hasValue(const std::string& constantName) const {
            return values.find(constantName) != values.end();
        }
        
        std::string getEnumName() const { return name; }
        AccessModifier getAccess() const { return access; }
        const std::map<std::string, int>& getValues() const { return values; }
    };
    
    // Class definition
    class MTypeClass : public std::enable_shared_from_this<MTypeClass> {
    private:
        std::string name;
        std::shared_ptr<MTypeClass> superClass;
        std::map<std::string, std::shared_ptr<MTypeFunction>> methods;
        std::map<std::string, std::shared_ptr<MTypeFunction>> staticMethods;
        std::map<std::string, Value> staticFields;
        std::map<std::string, ValueType> fieldTypes;
        std::map<std::string, Value> fieldDefaults;
        std::map<std::string, AccessModifier> memberAccess;
        std::shared_ptr<MTypeConstructor> constructor;
        bool isFinal;
        bool isAbstract;
        std::string documentation;
        
    public:
        MTypeClass(const std::string& n, 
                  std::shared_ptr<MTypeClass> super = nullptr,
                  bool final = false,
                  bool abstract = false)
            : name(n), superClass(super), isFinal(final), isAbstract(abstract) {}
        
        // Class hierarchy
        std::string getName() const { return name; }
        std::shared_ptr<MTypeClass> getSuperClass() const { return superClass; }
        bool hasSuperClass() const { return superClass != nullptr; }
        bool isSubclassOf(std::shared_ptr<MTypeClass> cls) const {
            if (!superClass) return false;
            if (superClass == cls) return true;
            return superClass->isSubclassOf(cls);
        }
        
        // Constructor
        void setConstructor(std::shared_ptr<MTypeConstructor> ctor) {
            constructor = ctor;
        }
        std::shared_ptr<MTypeConstructor> getConstructor() const {
            return constructor;
        }
        
        // Methods
        Result<void> defineMethod(const std::string& methodName,
                                 std::shared_ptr<MTypeFunction> method,
                                 AccessModifier access = AccessModifier::ACCESS_PUBLIC) {
            if (method->isStatic()) {
                staticMethods[methodName] = method;
            } else {
                methods[methodName] = method;
            }
            memberAccess[methodName] = access;
            return Result<void>::ok();
        }
        
        Result<std::shared_ptr<MTypeFunction>> findMethod(const std::string& methodName) const {
            // Check instance methods
            auto it = methods.find(methodName);
            if (it != methods.end()) {
                return Result<std::shared_ptr<MTypeFunction>>::ok(it->second);
            }
            
            // Check inherited methods
            if (superClass) {
                return superClass->findMethod(methodName);
            }
            
            return Result<std::shared_ptr<MTypeFunction>>::err(
                Error::name("Method '" + methodName + "' not found in class '" + name + "'"));
        }
        
        Result<std::shared_ptr<MTypeFunction>> findStaticMethod(const std::string& methodName) const {
            auto it = staticMethods.find(methodName);
            if (it != staticMethods.end()) {
                return Result<std::shared_ptr<MTypeFunction>>::ok(it->second);
            }
            
            if (superClass) {
                return superClass->findStaticMethod(methodName);
            }
            
            return Result<std::shared_ptr<MTypeFunction>>::err(
                Error::name("Static method '" + methodName + "' not found in class '" + name + "'"));
        }
        
        // Fields
        Result<void> defineField(const std::string& fieldName,
                                ValueType type,
                                const Value& defaultValue = Value::null(),
                                AccessModifier access = AccessModifier::ACCESS_PUBLIC) {
            fieldTypes[fieldName] = type;
            fieldDefaults[fieldName] = defaultValue;
            memberAccess[fieldName] = access;
            return Result<void>::ok();
        }
        
        Result<void> defineStaticField(const std::string& fieldName,
                                      const Value& value,
                                      AccessModifier access = AccessModifier::ACCESS_PUBLIC) {
            staticFields[fieldName] = value;
            memberAccess[fieldName] = access;
            return Result<void>::ok();
        }
        
        Result<Value> getStaticField(const std::string& fieldName) const {
            auto it = staticFields.find(fieldName);
            if (it != staticFields.end()) {
                return Result<Value>::ok(it->second);
            }
            
            if (superClass) {
                return superClass->getStaticField(fieldName);
            }
            
            return Result<Value>::err(
                Error::name("Static field '" + fieldName + "' not found in class '" + name + "'"));
        }
        
        Result<void> setStaticField(const std::string& fieldName, const Value& value) {
            auto it = staticFields.find(fieldName);
            if (it != staticFields.end()) {
                it->second = value;
                return Result<void>::ok();
            }
            
            if (superClass) {
                return superClass->setStaticField(fieldName, value);
            }
            
            return Result<void>::err(
                Error::name("Static field '" + fieldName + "' not found in class '" + name + "'"));
        }
        
        // Access control
        Result<AccessModifier> getMemberAccess(const std::string& memberName) const {
            auto it = memberAccess.find(memberName);
            if (it != memberAccess.end()) {
                return Result<AccessModifier>::ok(it->second);
            }
            
            if (superClass) {
                return superClass->getMemberAccess(memberName);
            }
            
            return Result<AccessModifier>::err(
                Error::name("Member '" + memberName + "' not found in class '" + name + "'"));
        }
        
        // Class properties
        bool getIsFinal() const { return isFinal; }
        bool getIsAbstract() const { return isAbstract; }
        void setDocumentation(const std::string& doc) { documentation = doc; }
        std::string getDocumentation() const { return documentation; }
        
        // Get all members (for reflection/debugging)
        std::vector<std::string> getMethodNames() const {
            std::vector<std::string> names;
            for (const auto& pair : methods) {
                names.push_back(pair.first);
            }
            return names;
        }
        
        std::vector<std::string> getFieldNames() const {
            std::vector<std::string> names;
            for (const auto& pair : fieldTypes) {
                names.push_back(pair.first);
            }
            return names;
        }
        
        const std::map<std::string, ValueType>& getFieldTypes() const { return fieldTypes; }
        const std::map<std::string, Value>& getFieldDefaults() const { return fieldDefaults; }
        
        std::string toString() const {
            return "<class " + name + ">";
        }
    };
    
    // Class instance
    class MTypeInstance : public std::enable_shared_from_this<MTypeInstance> {
    private:
        std::shared_ptr<MTypeClass> klass;
        std::map<std::string, Value> fields;
        
    public:
        explicit MTypeInstance(std::shared_ptr<MTypeClass> cls) : klass(cls) {
            // Initialize fields with defaults
            const auto& fieldTypes = klass->getFieldTypes();
            const auto& fieldDefaults = klass->getFieldDefaults();
            
            for (const auto& [fieldName, fieldType] : fieldTypes) {
                auto it = fieldDefaults.find(fieldName);
                if (it != fieldDefaults.end()) {
                    fields[fieldName] = it->second;
                } else {
                    fields[fieldName] = Value::null();
                }
            }
        }
        
        std::shared_ptr<MTypeClass> getClass() const { return klass; }
        
        // Field access
        Result<Value> getField(const std::string& fieldName) const {
            auto it = fields.find(fieldName);
            if (it != fields.end()) {
                return Result<Value>::ok(it->second);
            }
            
            return Result<Value>::err(
                Error::name("Field '" + fieldName + "' not found in instance of '" + 
                           klass->getName() + "'"));
        }
        
        Result<void> setField(const std::string& fieldName, const Value& value) {
            // Check if field exists in class definition
            const auto& fieldTypes = klass->getFieldTypes();
            auto typeIt = fieldTypes.find(fieldName);
            
            if (typeIt == fieldTypes.end()) {
                return Result<void>::err(
                    Error::name("Field '" + fieldName + "' not defined in class '" + 
                               klass->getName() + "'"));
            }
            
            // Type check
            if (typeIt->second != ValueType::V_NULL && value.type != typeIt->second) {
                return Result<void>::err(
                    Error::errorType("Field '" + fieldName + "' expects type " +
                                    Value::typeToString(typeIt->second) + ", got " +
                                    value.typeName()));
            }
            
            fields[fieldName] = value;
            return Result<void>::ok();
        }
        
        // Method access (returns bound method)
        Result<std::shared_ptr<MTypeBoundMethod>> getMethod(const std::string& methodName) {
            auto methodResult = klass->findMethod(methodName);
            if (methodResult.isError()) {
                return Result<std::shared_ptr<MTypeBoundMethod>>::err(methodResult.error());
            }
            
            auto method = methodResult.value();
            auto boundMethod = std::make_shared<MTypeBoundMethod>(method, shared_from_this());
            return Result<std::shared_ptr<MTypeBoundMethod>>::ok(boundMethod);
        }
        
        // Type checking
        bool isInstanceOf(std::shared_ptr<MTypeClass> cls) const {
            return klass == cls || klass->isSubclassOf(cls);
        }
        
        // Get all field names (for debugging/reflection)
        std::vector<std::string> getFieldNames() const {
            std::vector<std::string> names;
            for (const auto& pair : fields) {
                names.push_back(pair.first);
            }
            return names;
        }
        
        std::string toString() const {
            return "<instance of " + klass->getName() + ">";
        }
    };
    
    
}

