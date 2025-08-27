        import "tests/testFiles/import/pass/math_utils.mt";
        import "tests/testFiles/import/pass/string_utils.mt";
        import "tests/testFiles/import/pass/math_utils.mt";  // Safe duplicate - will be ignored
        
        native function print(int n): void;
        native function print(string s): void;
        
        // Use imported functions
        int sum = add(10, 20);
        print(sum);
        
        int product = multiply(5, 6);
        print(product);
        
        int processed = processNumber(7);
        print(processed);
        
        // Use imported variables
        print(MATH_CONSTANT);
        print(GREETING);