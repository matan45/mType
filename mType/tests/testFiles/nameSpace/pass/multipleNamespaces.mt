namespace math {
int pi = 3;

function add(int a, int b): int {
    return a + b;
}

function circleArea(int radius): int {
    return pi * radius * radius;
}
}

namespace utils {
string version = "1.0";

function formatNumber(int n): string {
    return "Value: " + toString(n);
}

function double(int x): int {
    return x * 2;
}
}

namespace constants {
int maxValue = 1000;
bool debugMode = true;

function getMax(): int {
    return maxValue;
}
}

// Test functions from different namespaces
int sum = math::add(15, 25);
print(sum);

int area = math::circleArea(5);
print(area);

int doubled = utils::double(21);
print(doubled);

int maximum = constants::getMax();
print(maximum);