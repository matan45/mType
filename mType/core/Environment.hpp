#pragma once
#include "Result.hpp"
#include "Value.hpp"
#include <string>
#include <map>
#include <memory>

namespace mtype {

	// Forward declaration
	class Namespace;
	namespace core {

		// Environment for variable scoping
		class Environment {
		private:
			std::map<std::string, Value> values;
			std::map<std::string, ValueType> types;
			std::map<std::string, AccessModifier> access;
			Environment* parent;
			std::weak_ptr<Namespace> owningNamespace;

		public:
			Environment(Environment* p = nullptr);
			Environment(Environment* p, std::shared_ptr<Namespace> ns);
			~Environment();

			// Variable management with Result types
			Result<void> define(const std::string& name, const Value& val,
				ValueType expectedType = ValueType::V_NULL,
				AccessModifier acc = AccessModifier::ACCESS_PUBLIC);

			Result<Value> get(const std::string& name) const;
			Result<void> set(const std::string& name, const Value& val);

			// Check if variable exists
			bool has(const std::string& name) const;
			bool hasLocal(const std::string& name) const;

			// Get type of variable
			Result<ValueType> getType(const std::string& name) const;

			// Access control
			Result<AccessModifier> getAccess(const std::string& name) const;
			Result<void> setAccess(const std::string& name, AccessModifier acc);

			// Parent environment access
			Environment* getParent() { return parent; }
			const Environment* getParent() const { return parent; }

			// Namespace access
			std::shared_ptr<Namespace> getNamespace() const;
			void setNamespace(std::shared_ptr<Namespace> ns);

			// Create child environment
			std::unique_ptr<Environment> createChild();

			// Debug/utility
			std::vector<std::string> getAllVariables() const;
			std::map<std::string, Value> getAllValues() const;

			// Special variables
			Result<void> defineThis(const Value& thisVal);
			Result<Value> getThis() const;
			bool hasThis() const;

			Result<void> defineSuper(const Value& superVal);
			Result<Value> getSuper() const;
			bool hasSuper() const;
		};

		// Scoped environment for automatic cleanup
		class ScopedEnvironment {
		private:
			Environment* parent;
			std::unique_ptr<Environment> env;

		public:
			ScopedEnvironment(Environment* p);
			~ScopedEnvironment() = default;

			Environment* operator->() { return env.get(); }
			const Environment* operator->() const { return env.get(); }
			Environment& operator*() { return *env; }
			const Environment& operator*() const { return *env; }

			Environment* get() { return env.get(); }
			const Environment* get() const { return env.get(); }
		};
	}

}
