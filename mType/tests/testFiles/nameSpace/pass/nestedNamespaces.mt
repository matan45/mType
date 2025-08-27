namespace math {
int baseValue = 10;

namespace algebra {
    int coefficient = 2;
    
    function linear(int x): int {
        return coefficient * x;
    }
    
    namespace advanced {
        function quadratic(int x): int {
            return x * x;
        }
    }
}

namespace geometry {
    function area(int width, int height): int {
        return width * height;
    }
}
}

// Test nested function calls
int linearResult = math::algebra::linear(5);
print(linearResult);

int quadResult = math::algebra::advanced::quadratic(4);
print(quadResult);

int areaResult = math::geometry::area(6, 7);
print(areaResult);
print("Test passed");