import "tests/testFiles/import/pass/math_utils.mt";
        
        function processNumber(int n): int {
            // Use function from imported file
            return multiply(n, 2);
        }
        
        string GREETING = "Hello";