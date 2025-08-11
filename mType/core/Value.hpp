#pragma once
#include <string>
#include <variant>
#include <memory>
#include <vector>
#include <map>
#include <functional>
#include "Result.hpp"

namespace mtype {
	// Forward declarations
	class Function;
	class MTypeClass;
	class MTypeInstance;
	class NativeFunction;
	class MTypeEnum;
	class ArrayValue;

	namespace core {

		enum class ValueType {
			V_INT, V_FLOAT, V_STRING, V_BOOL, V_NULL,
			V_FUNCTION, V_CLASS, V_INSTANCE,
			V_GENERIC, V_NATIVE_FUNCTION, V_ENUM, V_ARRAY
		};

		enum class AccessModifier {
			ACCESS_PUBLIC,
			ACCESS_PRIVATE,
			ACCESS_DEFAULT  // Default is public for backward compatibility
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
				std::shared_ptr<Function>,
				std::shared_ptr<MTypeClass>,
				std::shared_ptr<MTypeInstance>,
				std::shared_ptr<NativeFunction>,
				std::shared_ptr<MTypeEnum>,
				std::shared_ptr<ArrayValue>
			> data;

			// Constructors
			Value();
			Value(int i);
			Value(double d);
			Value(const std::string& s);
			Value(bool b);
			Value(std::shared_ptr<Function> f);
			Value(std::shared_ptr<MTypeClass> c);
			Value(std::shared_ptr<MTypeInstance> i);
			Value(std::shared_ptr<NativeFunction> nf);
			Value(std::shared_ptr<MTypeEnum> e);
			Value(std::shared_ptr<ArrayValue> arr);

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
			bool isArray() const { return type == ValueType::V_ARRAY; }
			bool isNumeric() const { return isInt() || isFloat(); }

			// Safe getters with Result
			Result<int> asInt() const;
			Result<double> asFloat() const;
			Result<std::string> asString() const;
			Result<bool> asBool() const;
			Result<std::shared_ptr<Function>> asFunction() const;
			Result<std::shared_ptr<MTypeClass>> asClass() const;
			Result<std::shared_ptr<MTypeInstance>> asInstance() const;
			Result<std::shared_ptr<ArrayValue>> asArray() const;

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

		// Enum class support
		class MTypeEnum {
		public:
			std::string name;
			std::map<std::string, int> values;
			std::map<int, std::string> reverseValues;
			AccessModifier access;

			MTypeEnum(const std::string& n, AccessModifier acc = AccessModifier::ACCESS_PUBLIC);

			Result<void> addValue(const std::string& constantName, int value);
			Result<int> getValue(const std::string& constantName) const;
			Result<std::string> getName(int value) const;
			bool hasValue(const std::string& constantName) const;
		};

		// Array value
		class ArrayValue {
		public:
			std::vector<Value> elements;
			ValueType elementType;
			int maxSize;  // -1 for dynamic arrays

			ArrayValue(ValueType elemType, int maxSz = -1);

			Result<void> push(const Value& val);
			Result<Value> get(int index) const;
			Result<void> set(int index, const Value& val);
			int length() const { return static_cast<int>(elements.size()); }

			Result<Value> pop();
			Result<void> insert(int index, const Value& val);
			Result<void> remove(int index);
			Result<int> indexOf(const Value& val) const;
			bool contains(const Value& val) const;

			std::vector<Value> toVector() const { return elements; }
		};

		// Native function wrapper
		class NativeFunction {
		public:
			std::string name;
			NativeFunctionPtr function;
			size_t arity;  // Number of parameters
			std::vector<ValueType> paramTypes;
			ValueType returnType;
			std::string documentation;

			NativeFunction(const std::string& n, NativeFunctionPtr f, size_t a,
				const std::vector<ValueType>& pt = {},
				ValueType rt = ValueType::V_NULL,
				const std::string& doc = "");

			Result<Value> call(const std::vector<Value>& args) const;
			Result<void> validateArgs(const std::vector<Value>& args) const;
		};
	}

}
