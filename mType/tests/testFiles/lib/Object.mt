// Object - Root base class for all objects in mType
// All classes implicitly inherit from Object — no explicit extends needed.
//
// NOTE: This file is a documentation stub. The actual Object class is registered
// as a built-in by ObjectClassBootstrap.cpp, and its methods are implemented
// natively in ObjectExecutor.cpp. This file exists solely as a language-level
// reference for the Object API contract.
//
// Default method behaviors (implemented in C++):
//   toString()  — returns "ClassName@hashCode" via ObjectInstance::getContentHash()
//   equals()    — content-based equality via ObjectInstance::contentEquals()
//   hashCode()  — content-based hash via ObjectInstance::getContentHash()

public class Object {

    // Returns a string representation of this object.
    // Default: "ClassName@hashCode"
    public function toString(): string{
	return "";
	}

    // Compares this object with another for equality.
    // Default: content-based equality (compares all field values recursively)
    public function equals(Object other): bool{
	return true;
	}

    // Returns a hash code for this object.
    // Default: content-based hash derived from all field values
    public function hashCode(): int{
	return 0;
	}
}
