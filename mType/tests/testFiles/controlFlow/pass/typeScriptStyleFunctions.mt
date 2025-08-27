        
        function greet(string name): void {
            print(42); 
        }
        
        function add(int a, int b): int {
            return a + b;
        }
        
        function divide(float a, float b): float {
            return a / b;
        }
        
        function isPositive(int x): bool {
            return x > 0;
        }
        
        greet("World");
        
        int sum = add(5, 10);
        print(sum);  // 15
        
        float result = divide(10.0, 3.0);
        print(result);  // 3.333...
        
        bool positive = isPositive(5);
        print(positive);  // 1 (true)
        
        bool negative = isPositive(-3);
        print(negative);  // 0 (false)
        
         print("test pass");