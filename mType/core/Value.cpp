#include "Value.hpp"
#include <sstream>
#include <stdexcept>
#include <cmath>
#include "../runtime/Function.hpp"
#include "../runtime/Klass.hpp"

namespace mtype::core {

	// Constructors
	Value::Value() : type(ValueType::V_NULL), data(nullptr) {}

	Value::Value(int i) : type(ValueType::V_INT), data(i) {}

	Value::Value(double d) : type(ValueType::V_FLOAT), data(d) {}

	Value::Value(const std::string& s) : type(ValueType::V_STRING), data(s) {}

	Value::Value(bool b) : type(ValueType::V_BOOL), data(b) {}

	Value::Value(std::shared_ptr<MTypeFunction> f) : type(ValueType::V_FUNCTION), data(f) {}

	Value::Value(std::shared_ptr<MTypeClass> c) : type(ValueType::V_CLASS), data(c) {}

	Value::Value(std::shared_ptr<MTypeInstance> i) : type(ValueType::V_INSTANCE), data(i) {}

	Value::Value(std::shared_ptr<MTypeNativeFunction> nf) : type(ValueType::V_NATIVE_FUNCTION), data(nf) {}

	Value::Value(std::shared_ptr<MTypeEnum> e) : type(ValueType::V_ENUM), data(e) {}

	// Static factory methods
	Value Value::null() {
		return Value();
	}

	Value Value::integer(int i) {
		return Value(i);
	}

	Value Value::floating(double d) {
		return Value(d);
	}

	Value Value::string(const std::string& s) {
		return Value(s);
	}

	Value Value::boolean(bool b) {
		return Value(b);
	}

	// Safe getters with Result
	Result<int> Value::asInt() const {
		if (type != ValueType::V_INT) {
			return Result<int>::err(Error::errorType("Expected int, got " + typeName()));
		}
		return Result<int>::ok(std::get<int>(data));
	}

	Result<double> Value::asFloat() const {
		if (type != ValueType::V_FLOAT) {
			return Result<double>::err(Error::errorType("Expected float, got " + typeName()));
		}
		return Result<double>::ok(std::get<double>(data));
	}

	Result<std::string> Value::asString() const {
		if (type != ValueType::V_STRING) {
			return Result<std::string>::err(Error::errorType("Expected string, got " + typeName()));
		}
		return Result<std::string>::ok(std::get<std::string>(data));
	}

	Result<bool> Value::asBool() const {
		if (type != ValueType::V_BOOL) {
			return Result<bool>::err(Error::errorType("Expected bool, got " + typeName()));
		}
		return Result<bool>::ok(std::get<bool>(data));
	}

	Result<std::shared_ptr<MTypeFunction>> Value::asFunction() const {
		if (type != ValueType::V_FUNCTION) {
			return Result<std::shared_ptr<MTypeFunction>>::err(Error::errorType("Expected function, got " + typeName()));
		}
		return Result<std::shared_ptr<MTypeFunction>>::ok(std::get<std::shared_ptr<MTypeFunction>>(data));
	}

	Result<std::shared_ptr<MTypeClass>> Value::asClass() const {
		if (type != ValueType::V_CLASS) {
			return Result<std::shared_ptr<MTypeClass>>::err(Error::errorType("Expected class, got " + typeName()));
		}
		return Result<std::shared_ptr<MTypeClass>>::ok(std::get<std::shared_ptr<MTypeClass>>(data));
	}

	Result<std::shared_ptr<MTypeInstance>> Value::asInstance() const {
		if (type != ValueType::V_INSTANCE) {
			return Result<std::shared_ptr<MTypeInstance>>::err(Error::errorType("Expected instance, got " + typeName()));
		}
		return Result<std::shared_ptr<MTypeInstance>>::ok(std::get<std::shared_ptr<MTypeInstance>>(data));
	}

	// Conversion methods
	std::string Value::toString() const {
		switch (type) {
		case ValueType::V_NULL:
			return "null";
		case ValueType::V_INT:
			return std::to_string(std::get<int>(data));
		case ValueType::V_FLOAT: {
			std::ostringstream oss;
			oss << std::get<double>(data);
			return oss.str();
		}
		case ValueType::V_STRING:
			return std::get<std::string>(data);
		case ValueType::V_BOOL:
			return std::get<bool>(data) ? "true" : "false";
		case ValueType::V_FUNCTION:
			return "<function>";
		case ValueType::V_CLASS:
			return "<class>";
		case ValueType::V_INSTANCE:
			return "<instance>";
		case ValueType::V_NATIVE_FUNCTION:
			return "<native function>";
		case ValueType::V_ENUM:
			return "<enum>";
		case ValueType::V_GENERIC:
			return "<generic>";
		default:
			return "<unknown>";
		}
	}

	bool Value::toBool() const {
		switch (type) {
		case ValueType::V_NULL:
			return false;
		case ValueType::V_BOOL:
			return std::get<bool>(data);
		case ValueType::V_INT:
			return std::get<int>(data) != 0;
		case ValueType::V_FLOAT:
			return std::get<double>(data) != 0.0;
		case ValueType::V_STRING:
			return !std::get<std::string>(data).empty();
		default:
			return true; // Objects, functions, etc. are truthy
		}
	}

	Result<double> Value::toNumeric() const {
		switch (type) {
		case ValueType::V_INT:
			return Result<double>::ok(static_cast<double>(std::get<int>(data)));
		case ValueType::V_FLOAT:
			return Result<double>::ok(std::get<double>(data));
		case ValueType::V_BOOL:
			return Result<double>::ok(std::get<bool>(data) ? 1.0 : 0.0);
		case ValueType::V_STRING: {
			try {
				const std::string& str = std::get<std::string>(data);
				size_t pos;
				double value = std::stod(str, &pos);
				if (pos != str.length()) {
					return Result<double>::err(Error::errorType("Invalid number format: " + str));
				}
				return Result<double>::ok(value);
			}
			catch (const std::exception&) {
				return Result<double>::err(Error::errorType("Cannot convert string to number: " + std::get<std::string>(data)));
			}
		}
		default:
			return Result<double>::err(Error::errorType("Cannot convert " + typeName() + " to number"));
		}
	}

	// Arithmetic operators
	Result<Value> Value::add(const Value& other) const {
		// String concatenation
		if (type == ValueType::V_STRING || other.type == ValueType::V_STRING) {
			return Result<Value>::ok(Value::string(toString() + other.toString()));
		}

		// Numeric addition
		if (isNumeric() && other.isNumeric()) {
			auto thisNum = toNumeric();
			auto otherNum = other.toNumeric();

			if (thisNum.isError()) return Result<Value>::err(thisNum.error());
			if (otherNum.isError()) return Result<Value>::err(otherNum.error());

			double result = thisNum.value() + otherNum.value();

			// Return int if both operands are int and result fits in int
			if (type == ValueType::V_INT && other.type == ValueType::V_INT) {
				if (result == static_cast<int>(result)) {
					return Result<Value>::ok(Value::integer(static_cast<int>(result)));
				}
			}
			return Result<Value>::ok(Value::floating(result));
		}

		return Result<Value>::err(Error::errorType("Cannot add " + typeName() + " and " + other.typeName()));
	}

	Result<Value> Value::subtract(const Value& other) const {
		if (!isNumeric() || !other.isNumeric()) {
			return Result<Value>::err(Error::errorType("Cannot subtract " + other.typeName() + " from " + typeName()));
		}

		auto thisNum = toNumeric();
		auto otherNum = other.toNumeric();

		if (thisNum.isError()) return Result<Value>::err(thisNum.error());
		if (otherNum.isError()) return Result<Value>::err(otherNum.error());

		double result = thisNum.value() - otherNum.value();

		if (type == ValueType::V_INT && other.type == ValueType::V_INT) {
			if (result == static_cast<int>(result)) {
				return Result<Value>::ok(Value::integer(static_cast<int>(result)));
			}
		}
		return Result<Value>::ok(Value::floating(result));
	}

	Result<Value> Value::multiply(const Value& other) const {
		if (!isNumeric() || !other.isNumeric()) {
			return Result<Value>::err(Error::errorType("Cannot multiply " + typeName() + " and " + other.typeName()));
		}

		auto thisNum = toNumeric();
		auto otherNum = other.toNumeric();

		if (thisNum.isError()) return Result<Value>::err(thisNum.error());
		if (otherNum.isError()) return Result<Value>::err(otherNum.error());

		double result = thisNum.value() * otherNum.value();

		if (type == ValueType::V_INT && other.type == ValueType::V_INT) {
			if (result == static_cast<int>(result)) {
				return Result<Value>::ok(Value::integer(static_cast<int>(result)));
			}
		}
		return Result<Value>::ok(Value::floating(result));
	}

	Result<Value> Value::divide(const Value& other) const {
		if (!isNumeric() || !other.isNumeric()) {
			return Result<Value>::err(Error::errorType("Cannot divide " + typeName() + " by " + other.typeName()));
		}

		auto thisNum = toNumeric();
		auto otherNum = other.toNumeric();

		if (thisNum.isError()) return Result<Value>::err(thisNum.error());
		if (otherNum.isError()) return Result<Value>::err(otherNum.error());

		if (otherNum.value() == 0.0) {
			return Result<Value>::err(Error::runtime("Division by zero"));
		}

		double result = thisNum.value() / otherNum.value();
		return Result<Value>::ok(Value::floating(result));
	}

	Result<Value> Value::modulo(const Value& other) const {
		if (type != ValueType::V_INT || other.type != ValueType::V_INT) {
			return Result<Value>::err(Error::errorType("Modulo operation requires integers"));
		}

		int otherInt = std::get<int>(other.data);
		if (otherInt == 0) {
			return Result<Value>::err(Error::runtime("Modulo by zero"));
		}

		int result = std::get<int>(data) % otherInt;
		return Result<Value>::ok(Value::integer(result));
	}

	// Comparison operators
	Result<bool> Value::equals(const Value& other) const {
		if (type != other.type) {
			return Result<bool>::ok(false);
		}

		switch (type) {
		case ValueType::V_NULL:
			return Result<bool>::ok(true);
		case ValueType::V_INT:
			return Result<bool>::ok(std::get<int>(data) == std::get<int>(other.data));
		case ValueType::V_FLOAT:
			return Result<bool>::ok(std::abs(std::get<double>(data) - std::get<double>(other.data)) < 1e-9);
		case ValueType::V_STRING:
			return Result<bool>::ok(std::get<std::string>(data) == std::get<std::string>(other.data));
		case ValueType::V_BOOL:
			return Result<bool>::ok(std::get<bool>(data) == std::get<bool>(other.data));
		default:
			// For objects, compare pointers
			return Result<bool>::ok(data == other.data);
		}
	}

	Result<bool> Value::notEquals(const Value& other) const {
		auto eq = equals(other);
		if (eq.isError()) return eq;
		return Result<bool>::ok(!eq.value());
	}

	Result<bool> Value::lessThan(const Value& other) const {
		if (!isNumeric() || !other.isNumeric()) {
			// String comparison
			if (type == ValueType::V_STRING && other.type == ValueType::V_STRING) {
				return Result<bool>::ok(std::get<std::string>(data) < std::get<std::string>(other.data));
			}
			return Result<bool>::err(Error::errorType("Cannot compare " + typeName() + " and " + other.typeName()));
		}

		auto thisNum = toNumeric();
		auto otherNum = other.toNumeric();

		if (thisNum.isError()) return Result<bool>::err(thisNum.error());
		if (otherNum.isError()) return Result<bool>::err(otherNum.error());

		return Result<bool>::ok(thisNum.value() < otherNum.value());
	}

	Result<bool> Value::lessEqual(const Value& other) const {
		auto lt = lessThan(other);
		auto eq = equals(other);

		if (lt.isError()) return lt;
		if (eq.isError()) return eq;

		return Result<bool>::ok(lt.value() || eq.value());
	}

	Result<bool> Value::greaterThan(const Value& other) const {
		auto le = lessEqual(other);
		if (le.isError()) return le;
		return Result<bool>::ok(!le.value());
	}

	Result<bool> Value::greaterEqual(const Value& other) const {
		auto lt = lessThan(other);
		if (lt.isError()) return lt;
		return Result<bool>::ok(!lt.value());
	}

	// Type name for error messages
	std::string Value::typeName() const {
		return typeToString(type);
	}

	std::string Value::typeToString(ValueType t) {
		switch (t) {
		case ValueType::V_INT: return "int";
		case ValueType::V_FLOAT: return "float";
		case ValueType::V_STRING: return "string";
		case ValueType::V_BOOL: return "bool";
		case ValueType::V_NULL: return "null";
		case ValueType::V_FUNCTION: return "function";
		case ValueType::V_CLASS: return "class";
		case ValueType::V_INSTANCE: return "instance";
		case ValueType::V_GENERIC: return "generic";
		case ValueType::V_NATIVE_FUNCTION: return "native_function";
		case ValueType::V_ENUM: return "enum";
		default: return "unknown";
		}
	}
}
