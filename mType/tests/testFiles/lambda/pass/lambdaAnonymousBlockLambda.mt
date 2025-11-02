// Lambda in anonymous block test
interface Function {
    function apply(int x) : int;
}

print("=== Anonymous Block Lambda Test ===");

Function result;
int blockVar = 50;

{
    // Anonymous block scope
    int localValue = 100;
    result = x -> {
        // Lambda captures variables from anonymous block
        return localValue + blockVar + x;
    };
}

print("Lambda result(10): " + result.apply(10));
print("Lambda result(25): " + result.apply(25));

// Nested anonymous blocks
Function nested;
{
    int outer = 10;
    {
        int middle = 20;
        {
            int inner = 30;
            nested = x -> outer + middle + inner + x;
        }
    }
}

print("Nested(5): " + nested.apply(5));

// Multiple lambdas from different blocks
Function[] lambdas = new Function[3];
{
    int val = 1;
    lambdas[0] = x -> val + x;
}
{
    int val = 2;
    lambdas[1] = x -> val + x;
}
{
    int val = 3;
    lambdas[2] = x -> val + x;
}

for (int i = 0; i < 3; i = i + 1) {
    print("lambdas[" + i + "](10) = " + lambdas[i].apply(10));
}

print("Anonymous block lambda complete");
