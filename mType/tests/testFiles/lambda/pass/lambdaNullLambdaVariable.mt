// Assigning null to interface variable test
interface Function {
    function apply(int x) : int;
}

print("=== Null Lambda Variable Test ===");

Function func = x -> x * 2;
print("Initial: " + func.apply(5));

// Assign null to lambda variable
func = null;
print("After null assignment: " + (func == null ? "null" : "not null"));

// Null check before invocation
Function? maybeNull = null;

if (maybeNull == null) {
    print("Lambda is null, creating new one");
    maybeNull = x -> x + 10;
}

print("Result: " + maybeNull.apply(5));

// Array of lambdas with null
Function[] funcs = new Function[5];
funcs[0] = x -> x + 1;
funcs[1] = null;
funcs[2] = x -> x * 2;
funcs[3] = null;
funcs[4] = x -> x - 1;

for (int i = 0; i < 5; i = i + 1) {
    if (funcs[i] == null) {
        print("funcs[" + i + "] is null");
    } else {
        print("funcs[" + i + "](10) = " + funcs[i].apply(10));
    }
}

// Conditional lambda assignment
bool useMultiply = false;
Function conditional = useMultiply ? (x -> x * 3) : null;

if (conditional == null) {
    print("Conditional is null");
    conditional = x -> x + 5;
}

print("Conditional result: " + conditional.apply(10));

print("Null lambda variable complete");
