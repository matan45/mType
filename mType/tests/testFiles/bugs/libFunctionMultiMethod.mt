// MYT-221: lib/functional/Function.mt isn't a single-method functional interface.
//
// EXPECTED:
//   `Function<Int, Int> f = x -> ...;` works. Function is a SAM interface,
//   matching the canonical idiom from every other language with this type.
//
// ACTUAL (broken):
//   Type error: Cannot assign lambda to non-functional interface 'Function'.
//   Lambdas can only be assigned to interfaces with exactly one method.
//   Interface 'Function' has 3 methods.
//
// Function declares apply + andThen + compose; BiFunction declares apply +
// andThen. Either andThen/compose should be default methods (Java 8 style)
// or the names should be split across SAM + composition-helper interfaces.

import * from "../../lib/functional/Function.mt";
import * from "../../lib/primitives/Int.mt";

Function<Int, Int> doubler = x -> new Int(x.getValue() * 2);
print("doubler(5) = " + doubler.apply(new Int(5)).getValue());
