// Test: Polymorphism with async interface methods
// @Script

interface AsyncCalculator {
    async calculate(a: Int, b: Int) : Promise<Int>;
    async getName() : Promise<String>;
}

class Adder : AsyncCalculator {
    async calculate(a: Int, b: Int) : Promise<Int> {
        await delay(5);
        return a + b;
    }

    async getName() : Promise<String> {
        await delay(5);
        return "Adder";
    }
}

class Multiplier : AsyncCalculator {
    async calculate(a: Int, b: Int) : Promise<Int> {
        await delay(5);
        return a * b;
    }

    async getName() : Promise<String> {
        await delay(5);
        return "Multiplier";
    }
}

class Subtractor : AsyncCalculator {
    async calculate(a: Int, b: Int) : Promise<Int> {
        await delay(5);
        return a - b;
    }

    async getName() : Promise<String> {
        await delay(5);
        return "Subtractor";
    }
}

async delay(ms: Int) : Promise<Void> {
    // Simulated delay
}

async performOperation(calc: AsyncCalculator, x: Int, y: Int) : Promise<String> {
    let name = await calc.getName();
    let result = await calc.calculate(x, y);
    return name + ": " + result.toString();
}

async main() : Promise<Void> {
    let calculators: AsyncCalculator[] = [];
    calculators.push(new Adder());
    calculators.push(new Multiplier());
    calculators.push(new Subtractor());

    // Test polymorphic async calls
    let i: Int = 0;
    while (i < calculators.length()) {
        let result = await performOperation(calculators[i], 10, 5);
        print(result);
        i = i + 1;
    }

    // Specific tests
    let adder = new Adder();
    let addResult = await adder.calculate(7, 3);
    assert(addResult == 10, "Adder should add correctly");

    let multiplier = new Multiplier();
    let multResult = await multiplier.calculate(7, 3);
    assert(multResult == 21, "Multiplier should multiply correctly");

    let subtractor = new Subtractor();
    let subResult = await subtractor.calculate(7, 3);
    assert(subResult == 4, "Subtractor should subtract correctly");
}
