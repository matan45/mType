// Performance test with large namespace structures
final int OPERATIONS_COUNT = 50;

int counter1 = 0;
function increment1(): int { counter1 = counter1 + 1; return counter1; }

int counter2 = 0;
function increment2(): int { counter2 = counter2 + 1; return counter2; }

int counter3 = 0;
function increment3(): int { counter3 = counter3 + 1; return counter3; }

int counter4 = 0;
function increment4(): int { counter4 = counter4 + 1; return counter4; }

int counter5 = 0;
function increment5(): int { counter5 = counter5 + 1; return counter5; }

function performanceBenchmark(): int {
    int totalOperations = 0;
    
    for (int i = 0; i < OPERATIONS_COUNT; i++) {
        increment1();
        increment2();
        increment3();
        increment4();
        increment5();
        totalOperations = totalOperations + 5;
    }
    
    return totalOperations;
}

// Run performance test
int operations = performanceBenchmark();
print(operations);
print(counter1);
print(counter2);
print(counter3);
print(counter4);
print(counter5);