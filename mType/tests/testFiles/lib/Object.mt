// Object - root of the class hierarchy.
//
// Declares the four methods every reference type ought to support so they
// resolve at type-check time:
//   * toString  — human-readable form
//   * equals    — value-equality, default reference equality
//   * hashCode  — integer hash, default identity
//   * getClass  — runtime class name as a string
//
// Arrays widen to Object via MYT-281 and dispatch these methods through a
// runtime intercept (ObjectExecutor::invokeArrayObjectMethod) — they do
// not need to inherit Object at the source level. This file exists so the
// type checker has methods to resolve when a script writes `obj.toString()`
// against an Object-typed expression.
public class Object {
    public function toString(): string {
        return "Object";
    }

    public function equals(Object other): bool {
        return this == other;
    }

    public function hashCode(): int {
        return 0;
    }

    public function getClass(): string {
        return "Object";
    }
}
