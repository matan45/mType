// Tier B: auto-box / auto-unbox path returns early when the box class name
// matches the primitive. Verify the early return does not also accept a
// MISMATCHED box class.
// Targets: StatementCompiler.cpp:881-888 (declaration auto-box) and the
//          mirroring auto-unbox path TypeValidator.cpp:354-364.
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// String wrapper assigned to int primitive must error — the auto-unbox path
// only accepts Int -> int / Float -> float / Bool -> bool / String -> string.
String s = new String("hello");
int wrong = s;

print(wrong);
