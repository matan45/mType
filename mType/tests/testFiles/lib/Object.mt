// Object - Root base class for all objects in mType
// All classes implicitly inherit from Object — no explicit extends needed.
// Provides default implementations for toString, equals, and hashCode.
// These defaults are backed by native C++ implementations in the runtime.

public class Object {

    // Returns a string representation of this object.
    // Default: "ClassName@hashCode"
    public function toString(): string {
        return parsePrimitive(hashCode(this));
    }

    // Compares this object with another for equality.
    // Default: content-based equality (compares all field values recursively)
    public function equals(Object other): bool {
        return this == other;
    }

    // Returns a hash code for this object.
    // Default: content-based hash derived from all field values
    public function hashCode(): int {
        return hashCode(this);
    }
}
