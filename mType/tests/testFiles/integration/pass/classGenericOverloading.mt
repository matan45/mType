// Class + Generics Test 3: Generic method overloading in classes
@Script

class Processor<T> {
    T value;

    constructor(T val) {
        this.value = val;
    }

    public function process(): T {
        return this.value;
    }

    public function process(T modifier): T {
        return modifier;
    }

    public function process(T modifier1, T modifier2): T {
        return modifier1;
    }

    public function getValue(): T {
        return this.value;
    }
}

class Calculator {
    public function compute(Int a): Int {
        return a * 2;
    }

    public function compute(Int a, Int b): Int {
        return a + b;
    }

    public function compute(Int a, Int b, Int c): Int {
        return a + b + c;
    }

    public function compute(Float a): Float {
        return a * 2.0;
    }

    public function compute(Float a, Float b): Float {
        return a + b;
    }
}

print("Testing Processor<Int>:");
Processor<Int> intProc = Processor<Int>(10);
print(intProc.process());
print(intProc.process(20));
print(intProc.process(30, 40));
print(intProc.getValue());

print("Testing Processor<String>:");
Processor<String> strProc = Processor<String>("original");
print(strProc.process());
print(strProc.process("modified"));
print(strProc.process("mod1", "mod2"));

print("Testing Calculator:");
Calculator calc = Calculator();
print(calc.compute(5));
print(calc.compute(5, 10));
print(calc.compute(5, 10, 15));
print(calc.compute(2.5));
print(calc.compute(2.5, 3.5));

print("Testing Processor<Bool>:");
Processor<Bool> boolProc = Processor<Bool>(true);
print(boolProc.process());
print(boolProc.process(false));
print(boolProc.process(true, false));
