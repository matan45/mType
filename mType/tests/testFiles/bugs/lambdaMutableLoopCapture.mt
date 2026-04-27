// MYT-215: Lambda captures mutable for-loop variable by reference; no diagnostic.
//
// EXPECTED (one of two — both are documented closure semantics):
//   1. Java-style: compile error "loop variable must be effectively final"
//   2. Kotlin / modern-JS-let-style: each iteration captures by-value
//      → funcs[0](10)=10, funcs[1](10)=11, funcs[2](10)=12
//
// ACTUAL (broken — worst-of-both-worlds):
//   Compiles silently. Captures by reference. All three lambdas observe
//   the post-loop value of i (3):
//     funcs[0](10)=13, funcs[1](10)=13, funcs[2](10)=13

interface Function {
    function apply(int x): int;
}

Function[] funcs = new Function[3];

for (int i = 0; i < 3; i = i + 1) {
    funcs[i] = x -> i + x;
}

print("funcs[0](10) = " + funcs[0].apply(10));
print("funcs[1](10) = " + funcs[1].apply(10));
print("funcs[2](10) = " + funcs[2].apply(10));
