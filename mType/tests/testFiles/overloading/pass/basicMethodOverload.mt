// Basic method overloading in classes
class Calculator {
    public function add(int a, int b): int {
        return a + b;
    }

    public function add(float a, float b): float {
        return a + b;
    }

    public function add(string a, string b): string {
        return a + b;
    }

    public function add(int a, int b, int c): int {
        return a + b + c;
    }
}


function main(): void {
    Calculator calc = new Calculator();

    int result1 = calc.add(5, 10);          // Should call add(int, int)
    print("5 + 10 = " + result1);

    float result2 = calc.add(3.5, 2.5);     // Should call add(float, float)
    print("3.5 + 2.5 = " + result2);

    string result3 = calc.add("Hello", " World");  // Should call add(String, String)
    print(result3);

    int result4 = calc.add(1, 2, 3);        // Should call add(int, int, int)
    print("1 + 2 + 3 = " + result4);
}
main();
