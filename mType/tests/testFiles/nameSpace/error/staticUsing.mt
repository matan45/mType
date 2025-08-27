namespace math {
    function add(int a, int b): int {
        return a + b;
    }
}

// Static modifier cannot be applied to using directives
static using namespace math;

// This code should never be reached due to parse error
print("This should not print");