// @Data on a class with a NON-int field (string) now produces structural
// equals/hashCode via the opt-in broadened synthesis gate, instead of silently
// falling back to reference equality. Two structurally-equal Users compare
// equal and hash identically; a differing one does not.
//
// The synthesized hashCode/equals box the string field to call its
// hashCode()/equals(), so the String primitive class must be loaded.
import * from "../../lib/primitives/String.mt";

@Data
class User {
    private final string name;
    private final int age;
}

User a = new User("Alice", 30);
User b = new User("Alice", 30);
User c = new User("Bob", 30);
print(a.equals(b));
print(a.equals(c));
print(a.hashCode() == b.hashCode());
print(a.toString());
