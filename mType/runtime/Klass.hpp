#pragma once
#include "Function.hpp"

namespace mtype::runtime
{
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

	class MTypeKlass
	{
	};

	class MTypeInstance
	{
		
	};
}

