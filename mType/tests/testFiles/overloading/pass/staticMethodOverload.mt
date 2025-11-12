// Static method overloading
class MathUtils {
    public static function max(int a, int b): int {
        if (a > b) {
            return a;
        }
        return b;
    }

    public static function max(float a, float b): float {
        if (a > b) {
            return a;
        }
        return b;
    }

    public static function max(int a, int b, int c): int {
        int temp = a;
        if (b > temp) {
            temp = b;
        }
        if (c > temp) {
            temp = c;
        }
        return temp;
    }
}


function main(): void {
    int result1 = MathUtils::max(10, 20);       // Should call max(int, int)
    print("max(10, 20) = " + result1);

    float result2 = MathUtils::max(3.5, 2.1);   // Should call max(float, float)
    print("max(3.5, 2.1) = " + result2);

    int result3 = MathUtils::max(5, 15, 10);    // Should call max(int, int, int)
    print("max(5, 15, 10) = " + result3);
}
main();
