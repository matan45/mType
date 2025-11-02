// Test: Polymorphic async with generics and lambdas
// @Script

import "modules/PolymorphicAsyncInterfaces.mt";

class DataHandler<T> : AsyncComposite<T> {
    private validator: (T) -> Bool;
    private handler: (T) -> T;

    constructor(v: (T) -> Bool, h: (T) -> T) {
        this.validator = v;
        this.handler = h;
    }

    async validate(input: T) : Promise<Bool> {
        await delay(5);
        return this.validator(input);
    }

    async handle(input: T) : Promise<T> {
        await delay(5);
        return this.handler(input);
    }

    async process(input: T, onValid: (T) -> T) : Promise<T> {
        await delay(5);
        let isValid = await this.validate(input);
        if (isValid) {
            let handled = await this.handle(input);
            return onValid(handled);
        }
        return input;
    }
}

async processWithInterface<T>(
    handler: AsyncComposite<T>,
    data: T,
    transformer: (T) -> T
) : Promise<T> {
    let validated = await handler.validate(data);
    if (validated) {
        return await handler.process(data, transformer);
    }
    return data;
}

async delay(ms: Int) : Promise<Void> {
    // Simulated delay
}

async main() : Promise<Void> {
    // String handler
    let strHandler = new DataHandler<String>(
        (s: String) : Bool => { return s.length() > 0; },
        (s: String) : String => { return s + "!"; }
    );

    let result1 = await strHandler.process(
        "Hello",
        (s: String) : String => { return s + "?"; }
    );
    print("String result: " + result1);
    assert(result1 == "Hello!?", "Should process string correctly");

    // Int handler
    let intHandler = new DataHandler<Int>(
        (n: Int) : Bool => { return n > 0; },
        (n: Int) : Int => { return n * 2; }
    );

    let result2 = await intHandler.process(
        5,
        (n: Int) : Int => { return n + 10; }
    );
    print("Int result: " + result2.toString());
    assert(result2 == 20, "Should process int correctly"); // (5 * 2) + 10 = 20

    // Polymorphic usage
    let result3 = await processWithInterface<String>(
        strHandler,
        "Test",
        (s: String) : String => { return "[" + s + "]"; }
    );
    print("Polymorphic result: " + result3);
    assert(result3 == "[Test!]", "Should work polymorphically");

    print("Polymorphic async test passed");
}
