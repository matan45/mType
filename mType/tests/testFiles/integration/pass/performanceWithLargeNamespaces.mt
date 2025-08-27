// Performance test with large namespace structures
namespace performanceTest {
    final int OPERATIONS_COUNT = 50;
    
    namespace module1 {
        int counter1 = 0;
        function increment1(): int { counter1 = counter1 + 1; return counter1; }
    }
    
    namespace module2 {
        int counter2 = 0;
        function increment2(): int { counter2 = counter2 + 1; return counter2; }
    }
    
    namespace module3 {
        int counter3 = 0;
        function increment3(): int { counter3 = counter3 + 1; return counter3; }
    }
    
    namespace module4 {
        int counter4 = 0;
        function increment4(): int { counter4 = counter4 + 1; return counter4; }
    }
    
    namespace module5 {
        int counter5 = 0;
        function increment5(): int { counter5 = counter5 + 1; return counter5; }
    }
    
    function performanceBenchmark(): int {
        int totalOperations = 0;
        
        for (int i = 0; i < OPERATIONS_COUNT; i++) {
            module1::increment1();
            module2::increment2();
            module3::increment3();
            module4::increment4();
            module5::increment5();
            totalOperations = totalOperations + 5;
        }
        
        return totalOperations;
    }
}

// Run performance test
int operations = performanceTest::performanceBenchmark();
print(operations);
print(performanceTest::module1::counter1);
print(performanceTest::module2::counter2);
print(performanceTest::module3::counter3);
print(performanceTest::module4::counter4);
print(performanceTest::module5::counter5);