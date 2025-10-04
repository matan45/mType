// Test interfaces as function parameters
interface Function {
    function apply(int x) : int;
}

interface BinaryFunction {
    function apply(int a, int b) : int;
}

interface Predicate {
    function test(int x) : bool;
}

interface Consumer {
    function accept(int value) : void;
}

interface Runnable {
    function run() : void;
}

// Class that implements Function interface
class Doubler implements Function {
    public function apply(int x) : int {
        return x * 2;
    }
}

// Class that implements Function interface with state
class Multiplier implements Function {
    int factor;

    public constructor(int f) {
        factor = f;
    }

    public function apply(int x) : int {
        return x * factor;
    }
}

// Class that implements BinaryFunction interface
class Calculator implements BinaryFunction {
    public function apply(int a, int b) : int {
        return a + b * 2;
    }
}

// Class that implements Predicate interface
class EvenChecker implements Predicate {
    public function test(int x) : bool {
        return (x % 2) == 0;
    }
}

class FunctionalProcessor {
    // Method that accepts Function interface as parameter
    public function processWithFunction(Function f, int value) : int {
        return f.apply(value);
    }

    // Method that accepts BinaryFunction interface as parameter
    public function processWithBinaryFunction(BinaryFunction bf, int a, int b) : int {
        return bf.apply(a, b);
    }

    // Method that accepts Predicate interface as parameter
    public function filterValue(Predicate pred, int value) : bool {
        return pred.test(value);
    }

    // Method that accepts Consumer interface as parameter
    public function consumeValue(Consumer consumer, int value) : void {
        consumer.accept(value);
    }

    // Method that accepts Runnable interface as parameter
    public function executeTask(Runnable task) : void {
        task.run();
    }

    // Method that accepts multiple interface parameters
    public function complexOperation(Function mapper, Predicate filter, int value) : int {
        if (filter.test(value)) {
            return mapper.apply(value);
        }
        return -1;
    }

    // Methods that return interfaces implemented by class objects
    public function createDoubler() : Function {
        return new Doubler();
    }

    public function createMultiplier(int factor) : Function {
        return new Multiplier(factor);
    }

    public function createCalculator() : BinaryFunction {
        return new Calculator();
    }

    public function createEvenChecker() : Predicate {
        return new EvenChecker();
    }

    public function createFunctionBasedOnType(int type) : Function {
        if (type == 1) {
            return new Doubler();
        } else {
            return new Multiplier(type);
        }
    }
}

// Utility class with static functions that accept interfaces as parameters
class FunctionUtils {
    public static function applyTwice(Function f, int value) : int {
        return f.apply(f.apply(value));
    }

    public static function combineOperations(Function f1, Function f2, int value) : int {
        return f1.apply(value) + f2.apply(value);
    }

    public static function conditionalExecute(Predicate condition, Runnable task, int testValue) : void {
        if (condition.test(testValue)) {
            task.run();
        }
    }

    // Static factory methods that return interfaces implemented by class objects
    public static function createDefaultDoubler() : Function {
        return new Doubler();
    }

    public static function createCustomMultiplier(int factor) : Function {
        return new Multiplier(factor);
    }

    public static function createDefaultCalculator() : BinaryFunction {
        return new Calculator();
    }

    public static function createDefaultEvenChecker() : Predicate {
        return new EvenChecker();
    }

    public static function getOptimalFunction(int inputSize) : Function {
        if (inputSize < 10) {
            return new Doubler();  // Simple doubling for small inputs
        } else {
            return new Multiplier(3);  // Triple for larger inputs
        }
    }
}

print("=== Interface as Parameter Test ===");

FunctionalProcessor processor = new FunctionalProcessor();

// Test Function as parameter
Function doubler = x -> x * 2;
Function squarer = x -> x * x;
print("Process doubler (5): " + processor.processWithFunction(doubler, 5));
print("Process squarer (4): " + processor.processWithFunction(squarer, 4));

// Test with class implementations
Doubler doublerObj = new Doubler();
Multiplier tripler = new Multiplier(3);
Multiplier quintupler = new Multiplier(5);
print("Process doubler object (6): " + processor.processWithFunction(doublerObj, 6));
print("Process tripler object (4): " + processor.processWithFunction(tripler, 4));
print("Process quintupler object (3): " + processor.processWithFunction(quintupler, 3));

// Test BinaryFunction as parameter
BinaryFunction adder = (a, b) -> a + b;
BinaryFunction multiplier = (a, b) -> a * b;
print("Process adder (3, 7): " + processor.processWithBinaryFunction(adder, 3, 7));
print("Process multiplier (4, 6): " + processor.processWithBinaryFunction(multiplier, 4, 6));

// Test with class implementation
Calculator calc = new Calculator();
print("Process calculator object (2, 3): " + processor.processWithBinaryFunction(calc, 2, 3));

// Test Predicate as parameter
Predicate isEven = x -> (x % 2) == 0;
Predicate isPositive = x -> x > 0;
print("Filter even (8): " + processor.filterValue(isEven, 8));
print("Filter even (7): " + processor.filterValue(isEven, 7));
print("Filter positive (5): " + processor.filterValue(isPositive, 5));
print("Filter positive (-3): " + processor.filterValue(isPositive, -3));

// Test with class implementation
EvenChecker evenChecker = new EvenChecker();
print("Filter even checker (10): " + processor.filterValue(evenChecker, 10));
print("Filter even checker (9): " + processor.filterValue(evenChecker, 9));

// Test Consumer as parameter
Consumer printer = value -> print("Consumed value: " + value);
Consumer doubleprinter = x -> print("Double consumed: " + (x * 2));
processor.consumeValue(printer, 42);
processor.consumeValue(doubleprinter, 15);

// Test Runnable as parameter
Runnable task1 = () -> print("Task 1 executed!");
Runnable task2 = () -> print("Task 2 executed!");
processor.executeTask(task1);
processor.executeTask(task2);

// Test multiple interface parameters
Function lambdaTripler = x -> x * 3;
print("Complex operation (6): " + processor.complexOperation(lambdaTripler, isEven, 6));
print("Complex operation (5): " + processor.complexOperation(lambdaTripler, isEven, 5));

// Test static functions with interface parameters
print("Apply twice doubler (3): " + FunctionUtils::applyTwice(doubler, 3));
print("Apply twice squarer (3): " + FunctionUtils::applyTwice(squarer, 3));

print("Combine operations (4): " + FunctionUtils::combineOperations(doubler, squarer, 4));

// Test static functions with class objects
print("Apply twice doubler object (2): " + FunctionUtils::applyTwice(doublerObj, 2));
print("Apply twice tripler object (2): " + FunctionUtils::applyTwice(tripler, 2));
print("Combine class objects (3): " + FunctionUtils::combineOperations(doublerObj, tripler, 3));

// Test conditional execution
Predicate greaterThan5 = x -> x > 5;
Runnable successTask = () -> print("Condition met - task executed!");
Runnable failTask = () -> print("This should not print");

FunctionUtils::conditionalExecute(greaterThan5, successTask, 8);
FunctionUtils::conditionalExecute(greaterThan5, failTask, 3);

print("=== Testing Functions Returning Interface Objects ===");

// Test instance methods that return interface objects
Function factoryDoubler = processor.createDoubler();
Function factoryMultiplier = processor.createMultiplier(4);
BinaryFunction factoryCalculator = processor.createCalculator();
Predicate factoryEvenChecker = processor.createEvenChecker();

print("Factory doubler (7): " + factoryDoubler.apply(7));
print("Factory multiplier (6): " + factoryMultiplier.apply(6));
print("Factory calculator (3, 5): " + factoryCalculator.apply(3, 5));
print("Factory even checker (12): " + factoryEvenChecker.test(12));
print("Factory even checker (13): " + factoryEvenChecker.test(13));

// Test conditional factory method
Function conditionalFunc1 = processor.createFunctionBasedOnType(1);
Function conditionalFunc2 = processor.createFunctionBasedOnType(5);
print("Conditional function type 1 (8): " + conditionalFunc1.apply(8));
print("Conditional function type 5 (8): " + conditionalFunc2.apply(8));

// Test static factory methods
Function staticDoubler = FunctionUtils::createDefaultDoubler();
Function staticMultiplier = FunctionUtils::createCustomMultiplier(6);
BinaryFunction staticCalculator = FunctionUtils::createDefaultCalculator();
Predicate staticEvenChecker = FunctionUtils::createDefaultEvenChecker();

print("Static factory doubler (9): " + staticDoubler.apply(9));
print("Static factory multiplier (4): " + staticMultiplier.apply(4));
print("Static factory calculator (2, 7): " + staticCalculator.apply(2, 7));
print("Static factory even checker (16): " + staticEvenChecker.test(16));
print("Static factory even checker (17): " + staticEvenChecker.test(17));

// Test optimization-based factory
Function optimalSmall = FunctionUtils::getOptimalFunction(5);
Function optimalLarge = FunctionUtils::getOptimalFunction(15);
print("Optimal small input (3): " + optimalSmall.apply(3));
print("Optimal large input (3): " + optimalLarge.apply(3));

// Test using factory-created objects as parameters
print("Using factory objects as parameters:");
print("Apply twice factory doubler (2): " + FunctionUtils::applyTwice(factoryDoubler, 2));
print("Apply twice static multiplier (2): " + FunctionUtils::applyTwice(staticMultiplier, 2));
print("Combine factory objects (5): " + FunctionUtils::combineOperations(factoryDoubler, staticMultiplier, 5));

print("Interface as parameter test completed");