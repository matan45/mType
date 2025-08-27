namespace mathUtils {
    function add(int a, int b): int {
        return a + b;
    }

    function square(int x): int {
        return x * x;
    }
}

int ad = mathUtils::add(5, 3);
print(ad);
int sq = mathUtils::square(6);
print(sq);

namespace outer {
    int value = 42;

    namespace inner {
        function getValue(): int {
            print("inner");
            return 999;
        }
    }
}

// Call nested namespace function through assignment
int innerResult = outer::inner::getValue();
print(innerResult);

// Access namespace variable through assignment 
int outerValue = outer::value;
print(outerValue);
outer::value = 72;
print(outer::value);