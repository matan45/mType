// Test: Polymorphism with async interface methods
// @Script

interface AsyncCalculator {
    function async calculate(a: Int, b: Int) : Promise<Int>;
    function async getName() : Promise<String>;
}

class Adder implements AsyncCalculator {
    public function async calculate(a: Int, b: Int) : Promise<Int> {
        await delay(5);
        return a + b;
    }

    public function async getName() : Promise<String> {
        await delay(5);
        return "Adder";
    }
}

class Multiplier implements AsyncCalculator {
    public function async calculate(a: Int, b: Int) : Promise<Int> {
        await delay(5);
        return a * b;
    }

    public function async getName() : Promise<String> {
        await delay(5);
        return "Multiplier";
    }
}

class Subtractor implements AsyncCalculator {
    public function async calculate(a: Int, b: Int) : Promise<Int> {
        await delay(5);
        return a - b;
    }

    public function async getName() : Promise<String> {
        await delay(5);
        return "Subtractor";
    }
}

function async delay(ms: Int) : Promise<void> {
    // Simulated delay
}

function async performOperation(calc: AsyncCalculator, x: Int, y: Int) : Promise<String> {
    String name = await calc.getName();
    Int result = await calc.calculate(x, y);
    return name + ": " + result.toString();
}

function async main() : Promise<void> {
    AsyncCalculator[] calculators = [];
    calculators.push(new Adder());
    calculators.push(new Multiplier());
    calculators.push(new Subtractor());

    // Test polymorphic async calls
    Int i = 0;
    while (i < calculators.length()) {
        String result = await performOperation(calculators[i], 10, 5);
        print(result);
        i = i + 1;
    }

    // Specific tests
    Adder adder = new Adder();
    Int addResult = await adder.calculate(7, 3);
    assert(addResult == 10, "Adder should add correctly");

    Multiplier multiplier = new Multiplier();
    Int multResult = await multiplier.calculate(7, 3);
    assert(multResult == 21, "Multiplier should multiply correctly");

    Subtractor subtractor = new Subtractor();
    Int subResult = await subtractor.calculate(7, 3);
    assert(subResult == 4, "Subtractor should subtract correctly");
}
