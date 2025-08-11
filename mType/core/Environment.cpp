#include "Environment.hpp"
#include "../runtime/Function.hpp"
#include "../runtime/Klass.hpp"
#include "../runtime/Namespace.hpp"

namespace mtype::core {

	// Environment implementation
	Environment::Environment(Environment* p) : parent(p) {}

	Environment::Environment(Environment* p, std::shared_ptr<Namespace> ns)
		: parent(p), owningNamespace(ns) {
	}

	Environment::~Environment() = default;

	// Variable management with Result types
	Result<void> Environment::define(const std::string& name, const Value& val,
		ValueType expectedType, AccessModifier acc) {

		// Check if variable already exists in current scope
		if (hasLocal(name)) {
			return Result<void>::err(Error::name("Variable '" + name + "' already defined in current scope"));
		}

		// Type checking if expected type is specified
		if (expectedType != ValueType::V_NULL) {
			if (val.type != expectedType) {
				return Result<void>::err(Error::errorType(
					"Type mismatch: expected " + Value::typeToString(expectedType) +
					", got " + val.typeName()));
			}
		}

		// Store the variable
		values[name] = val;
		types[name] = val.type;
		access[name] = acc;

		return Result<void>::ok();
	}

	Result<Value> Environment::get(const std::string& name) const {
		// Check current scope first
		auto it = values.find(name);
		if (it != values.end()) {
			return Result<Value>::ok(it->second);
		}

		// Check parent scopes
		if (parent != nullptr) {
			return parent->get(name);
		}

		return Result<Value>::err(Error::name("Undefined variable '" + name + "'"));
	}

	Result<void> Environment::set(const std::string& name, const Value& val) {
		// Check current scope first
		auto it = values.find(name);
		if (it != values.end()) {
			// Type checking
			auto typeIt = types.find(name);
			if (typeIt != types.end() && typeIt->second != ValueType::V_NULL) {
				if (val.type != typeIt->second) {
					return Result<void>::err(Error::errorType(
						"Type mismatch: cannot assign " + val.typeName() +
						" to variable of type " + Value::typeToString(typeIt->second)));
				}
			}

			it->second = val;
			types[name] = val.type;
			return Result<void>::ok();
		}

		// Check parent scopes
		if (parent != nullptr) {
			return parent->set(name, val);
		}

		return Result<void>::err(Error::name("Undefined variable '" + name + "'"));
	}

	// Check if variable exists
	bool Environment::has(const std::string& name) const {
		if (hasLocal(name)) {
			return true;
		}
		return parent != nullptr && parent->has(name);
	}

	bool Environment::hasLocal(const std::string& name) const {
		return values.find(name) != values.end();
	}

	// Get type of variable
	Result<ValueType> Environment::getType(const std::string& name) const {
		auto it = types.find(name);
		if (it != types.end()) {
			return Result<ValueType>::ok(it->second);
		}

		if (parent != nullptr) {
			return parent->getType(name);
		}

		return Result<ValueType>::err(Error::name("Undefined variable '" + name + "'"));
	}

	// Access control
	Result<AccessModifier> Environment::getAccess(const std::string& name) const {
		auto it = access.find(name);
		if (it != access.end()) {
			return Result<AccessModifier>::ok(it->second);
		}

		if (parent != nullptr) {
			return parent->getAccess(name);
		}

		return Result<AccessModifier>::err(Error::name("Undefined variable '" + name + "'"));
	}

	Result<void> Environment::setAccess(const std::string& name, AccessModifier acc) {
		auto it = access.find(name);
		if (it != access.end()) {
			it->second = acc;
			return Result<void>::ok();
		}

		if (parent != nullptr) {
			return parent->setAccess(name, acc);
		}

		return Result<void>::err(Error::name("Undefined variable '" + name + "'"));
	}

	// Namespace access
	std::shared_ptr<Namespace> Environment::getNamespace() const {
		auto ns = owningNamespace.lock();
		if (ns) {
			return ns;
		}

		if (parent != nullptr) {
			return parent->getNamespace();
		}

		return nullptr;
	}

	void Environment::setNamespace(std::shared_ptr<Namespace> ns) {
		owningNamespace = ns;
	}

	// Create child environment
	std::unique_ptr<Environment> Environment::createChild() {
		return std::make_unique<Environment>(this);
	}

	// Debug/utility
	std::vector<std::string> Environment::getAllVariables() const {
		std::vector<std::string> vars;

		// Add local variables
		for (const auto& pair : values) {
			vars.push_back(pair.first);
		}

		// Add parent variables (avoiding duplicates)
		if (parent != nullptr) {
			auto parentVars = parent->getAllVariables();
			for (const auto& var : parentVars) {
				if (std::find(vars.begin(), vars.end(), var) == vars.end()) {
					vars.push_back(var);
				}
			}
		}

		return vars;
	}

	std::map<std::string, Value> Environment::getAllValues() const {
		std::map<std::string, Value> allValues;

		// Start with parent values (so local values can override)
		if (parent != nullptr) {
			allValues = parent->getAllValues();
		}

		// Add/override with local values
		for (const auto& pair : values) {
			allValues[pair.first] = pair.second;
		}

		return allValues;
	}

	// Special variables - 'this' reference
	Result<void> Environment::defineThis(const Value& thisVal) {
		return define("this", thisVal, ValueType::V_NULL, AccessModifier::ACCESS_PRIVATE);
	}

	Result<Value> Environment::getThis() const {
		return get("this");
	}

	bool Environment::hasThis() const {
		return has("this");
	}

	// Special variables - 'super' reference
	Result<void> Environment::defineSuper(const Value& superVal) {
		return define("super", superVal, ValueType::V_NULL, AccessModifier::ACCESS_PRIVATE);
	}

	Result<Value> Environment::getSuper() const {
		return get("super");
	}

	bool Environment::hasSuper() const {
		return has("super");
	}

	// ScopedEnvironment implementation
	ScopedEnvironment::ScopedEnvironment(Environment* p)
		: parent(p), env(std::make_unique<Environment>(p)) {
	}

	// Variable shadowing check
	bool Environment::shadows(const std::string& name) const {
		return hasLocal(name) && parent != nullptr && parent->has(name);
	}

	// Get all variables in current scope only
	std::vector<std::string> Environment::getLocalVariables() const {
		std::vector<std::string> vars;
		for (const auto& pair : values) {
			vars.push_back(pair.first);
		}
		return vars;
	}

	// Check access permissions
	Result<void> Environment::checkAccess(const std::string& name, AccessModifier requiredAccess) const {
		auto accessResult = getAccess(name);
		if (accessResult.isError()) {
			return Result<void>::err(accessResult.error());
		}

		AccessModifier varAccess = accessResult.value();

		// Public access is always allowed
		if (varAccess == AccessModifier::ACCESS_PUBLIC) {
			return Result<void>::ok();
		}

		// Private access only allowed in same scope
		if (varAccess == AccessModifier::ACCESS_PRIVATE) {
			if (!hasLocal(name)) {
				return Result<void>::err(Error::access("Cannot access private variable '" + name + "' from different scope"));
			}
		}

		// Protected access logic would go here (requires class context)
		// For now, treat protected like private
		if (varAccess == AccessModifier::ACCESS_PROTECTED) {
			if (!hasLocal(name)) {
				return Result<void>::err(Error::access("Cannot access protected variable '" + name + "' from different scope"));
			}
		}

		return Result<void>::ok();
	}

	// Find the environment that contains a variable
	Environment* Environment::findEnvironment(const std::string& name) {
		if (hasLocal(name)) {
			return this;
		}

		if (parent != nullptr) {
			return parent->findEnvironment(name);
		}

		return nullptr;
	}

	const Environment* Environment::findEnvironment(const std::string& name) const {
		if (hasLocal(name)) {
			return this;
		}

		if (parent != nullptr) {
			return parent->findEnvironment(name);
		}

		return nullptr;
	}

	// Get depth of variable (how many scopes up)
	Result<int> Environment::getVariableDepth(const std::string& name) const {
		if (hasLocal(name)) {
			return Result<int>::ok(0);
		}

		if (parent != nullptr) {
			auto parentDepth = parent->getVariableDepth(name);
			if (parentDepth.isOk()) {
				return Result<int>::ok(parentDepth.value() + 1);
			}
		}

		return Result<int>::err(Error::name("Variable '" + name + "' not found"));
	}
}
