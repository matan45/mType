#pragma once
#include <string>
#include <variant>
#include <memory>
#include <vector>
#include <functional>
#include "Result.hpp"

namespace mtype {
	// Forward declarations
	namespace runtime {
		class MTypeFunction;
		class MTypeClass;
		class MTypeInstance;
		class MTypeNativeFunction;
		class MTypeEnum;
	}


	namespace core {

		enum class ValueType {
			V_INT, V_FLOAT, V_STRING, V_BOOL, V_NULL,
			V_FUNCTION, V_CLASS, V_INSTANCE,
			V_GENERIC, V_NATIVE_FUNCTION, V_ENUM
		};

		enum class AccessModifier {
			ACCESS_PUBLIC,
			ACCESS_PRIVATE,
			ACCESS_PROTECTED
		};

		// Native function pointer type
		using NativeFunctionPtr = std::function<Result<class Value>(const std::vector<class Value>&)>;

		// Value class - represents any value in mType
		class Value {
		public:
			ValueType type;
			std::variant<
				int,
				double,
				std::string,
				bool,
				std::nullptr_t,
				std::shared_ptr<runtime::MTypeFunction>,
				std::shared_ptr<runtime::MTypeClass>,
				std::shared_ptr<runtime::MTypeInstance>,
				std::shared_ptr<runtime::MTypeNativeFunction>,
				std::shared_ptr<runtime::MTypeEnum>
			> data;

			// Constructors
			Value();
			explicit Value(int i);
			explicit Value(double d);
			explicit Value(const std::string& s);
			explicit Value(bool b);
			explicit Value(std::shared_ptr<runtime::MTypeFunction> f);
			explicit Value(std::shared_ptr<runtime::MTypeClass> c);
			explicit Value(std::shared_ptr<runtime::MTypeInstance> i);
			explicit Value(std::shared_ptr<runtime::MTypeNativeFunction> nf);
			explicit Value(std::shared_ptr<runtime::MTypeEnum> e);

			// Static factory methods
			static Value null();
			static Value integer(int i);
			static Value floating(double d);
			static Value string(const std::string& s);
			static Value boolean(bool b);

			// Type checking
			bool isNull() const { return type == ValueType::V_NULL; }
			bool isInt() const { return type == ValueType::V_INT; }
			bool isFloat() const { return type == ValueType::V_FLOAT; }
			bool isString() const { return type == ValueType::V_STRING; }
			bool isBool() const { return type == ValueType::V_BOOL; }
			bool isFunction() const { return type == ValueType::V_FUNCTION; }
			bool isClass() const { return type == ValueType::V_CLASS; }
			bool isInstance() const { return type == ValueType::V_INSTANCE; }
			bool isNumeric() const { return isInt() || isFloat(); }

			// Safe getters with Result
			Result<int> asInt() const;
			Result<double> asFloat() const;
			Result<std::string> asString() const;
			Result<bool> asBool() const;
			Result<std::shared_ptr<runtime::MTypeFunction>> asFunction() const;
			Result<std::shared_ptr<runtime::MTypeClass>> asClass() const;
			Result<std::shared_ptr<runtime::MTypeInstance>> asInstance() const;

			// Conversion methods
			std::string toString() const;
			bool toBool() const;
			Result<double> toNumeric() const;

			// Operators
			Result<Value> add(const Value& other) const;
			Result<Value> subtract(const Value& other) const;
			Result<Value> multiply(const Value& other) const;
			Result<Value> divide(const Value& other) const;
			Result<Value> modulo(const Value& other) const;

			Result<bool> equals(const Value& other) const;
			Result<bool> notEquals(const Value& other) const;
			Result<bool> lessThan(const Value& other) const;
			Result<bool> lessEqual(const Value& other) const;
			Result<bool> greaterThan(const Value& other) const;
			Result<bool> greaterEqual(const Value& other) const;

			// Type name for error messages
			std::string typeName() const;
			static std::string typeToString(ValueType t);
		};

		
	}

}
