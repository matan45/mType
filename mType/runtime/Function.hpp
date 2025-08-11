#pragma once
#include "../core/Value.hpp"

namespace mtype::runtime
{
	using namespace core;
	// Method modifiers
	enum MethodModifier {
		METHOD_NONE = 0,
		METHOD_ABSTRACT = 1 << 0,
		METHOD_VIRTUAL = 1 << 1,
		METHOD_OVERRIDE = 1 << 2,
		METHOD_FINAL = 1 << 3,
		METHOD_STATIC = 1 << 4
	};

	// Native function wrapper
	class MTypeNativeFunction {
	private:
		std::string name;
		NativeFunctionPtr function;
		size_t arity;  // Number of parameters
		std::vector<ValueType> paramTypes;
		ValueType returnType;
		std::string documentation;
	public:

		MTypeNativeFunction(const std::string& n, NativeFunctionPtr f, size_t a,
			const std::vector<ValueType>& pt = {},
			ValueType rt = ValueType::V_NULL,
			const std::string& doc = "");

		Result<Value> call(const std::vector<Value>& args) const;
		Result<void> validateArgs(const std::vector<Value>& args) const;
	};

	class MTypeFunction
	{
	public:

	};

}
