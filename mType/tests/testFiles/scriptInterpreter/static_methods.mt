// Class with static methods and fields
class MathUtils {
    static final float PI = 3.14159;
    static int operationCount = 0;
    
    static function square(int x): int {
        MathUtils::operationCount = MathUtils::operationCount + 1;
        return x * x;
    }
    
    static function circleArea(float radius): float {
        MathUtils::operationCount = MathUtils::operationCount + 1;
        return MathUtils::PI * radius * radius;
    }
    
    static function getOperationCount(): int {
        return MathUtils::operationCount;
    }
    
    static function resetCount(): void {
        MathUtils::operationCount = 0;
    }
}

// Another class with static utilities
class StringUtils {
    static function reverse(string str): string {
        // Simple reverse implementation for demo
        return str + " (reversed)";  // Simplified for demo
    }
    
    static function repeat(string str, int times): string {
        string result = "";
        for (int i = 0; i < times; i = i + 1) {
            result = result + str;
        }
        return result;
    }
}