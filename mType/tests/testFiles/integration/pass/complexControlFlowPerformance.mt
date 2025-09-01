// Complex control flow performance test

    function complexAlgorithm(int n): int {
        int result = 0;
        
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n / 2; j++) {
                if (i % 2 == 0) {
                    if (j % 3 == 0) {
                        result = result + i + j;
                    } else {
                        result = result + i - j;
                    }
                } else {
                    if (j % 2 == 0) {
                        result = result + i * 2;
                    } else {
                        result = result + j * 2;
                    }
                }
                
                // Add some break/continue logic
                if (result > 1000) {
                    break;
                }
            }
            
            if (result > 2000) {
                break;
            }
        }
        
        return result;
    }


// Test complex control flow
int result1 = complexAlgorithm(10);
int result2 = complexAlgorithm(15);

print(result1);
print(result2);