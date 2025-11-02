// Class + Generics Test 3: Generic method overloading in classes
@Script

class Processor<T> {
    field value: T;

    constructor(val: T) {
        this.value = val;
    }

    fun process(): T {
        return this.value;
    }

    fun process(modifier: T): T {
        return modifier;
    }

    fun process(modifier1: T, modifier2: T): T {
        return modifier1;
    }

    fun getValue(): T {
        return this.value;
    }
}

class Calculator {
    fun compute(a: Int): Int {
        return a * 2;
    }

    fun compute(a: Int, b: Int): Int {
        return a + b;
    }

    fun compute(a: Int, b: Int, c: Int): Int {
        return a + b + c;
    }

    fun compute(a: Float): Float {
        return a * 2.0;
    }

    fun compute(a: Float, b: Float): Float {
        return a + b;
    }
}

print("Testing Processor<Int>:");
let intProc: Processor<Int> = Processor<Int>(10);
print(intProc.process());
print(intProc.process(20));
print(intProc.process(30, 40));
print(intProc.getValue());

print("Testing Processor<String>:");
let strProc: Processor<String> = Processor<String>("original");
print(strProc.process());
print(strProc.process("modified"));
print(strProc.process("mod1", "mod2"));

print("Testing Calculator:");
let calc: Calculator = Calculator();
print(calc.compute(5));
print(calc.compute(5, 10));
print(calc.compute(5, 10, 15));
print(calc.compute(2.5));
print(calc.compute(2.5, 3.5));

print("Testing Processor<Bool>:");
let boolProc: Processor<Bool> = Processor<Bool>(true);
print(boolProc.process());
print(boolProc.process(false));
print(boolProc.process(true, false));
