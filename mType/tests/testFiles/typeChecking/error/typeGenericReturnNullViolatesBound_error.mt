// Tier B: function with a generic return type T extending a non-nullable
// bound returns `null`. The "isGenericReturnType skip" at
// FunctionCompiler.cpp:305 was designed for unbound T (where T may be
// nullable at instantiation) — verify it does not also bypass when the
// declared bound itself is non-nullable.
// Targets: FunctionCompiler.cpp:303-313.

class Animal {
    public constructor() {}
}

// T bounded by non-nullable Animal — null is not a valid Animal value.
function <T extends Animal> producesNonNull(): T {
    return null;
}

Animal a = producesNonNull<Animal>();
print("should not reach");
