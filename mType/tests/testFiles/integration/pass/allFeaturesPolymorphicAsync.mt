// Test: Polymorphic async with generics and lambdas
// @Script

import "modules/PolymorphicAsyncInterfaces.mt";

class DataHandler<T> implements AsyncComposite<T> {
    private (T) -> Bool validator;
    private (T) -> T handler;

    constructor((T) -> Bool v, (T) -> T h) {
        this.validator = v;
        this.handler = h;
    }

    function async validate(T input) : Promise<Bool> {
        await delay(5);
        return this.validator(input);
    }

    function async handle(T input) : Promise<T> {
        await delay(5);
        return this.handler(input);
    }

    function async process(T input, (T) -> T onValid) : Promise<T> {
        await delay(5);
        Bool isValid = await this.validate(input);
        if (isValid) {
            T handled = await this.handle(input);
            return onValid(handled);
        }
        return input;
    }
}

function async processWithInterface<T>(
    AsyncComposite<T> handler,
    T data,
    (T) -> T transformer
) : Promise<T> {
    Bool validated = await handler.validate(data);
    if (validated) {
        return await handler.process(data, transformer);
    }
    return data;
}

function async delay(Int ms) : Promise<void> {
    // Simulated delay
}

function async main() : Promise<void> {
    // String handler
    DataHandler<String> strHandler = new DataHandler<String>(
        (String s) : Bool => { return s.length() > 0; },
        (String s) : String => { return s + "!"; }
    );

    String result1 = await strHandler.process(
        "Hello",
        (String s) : String => { return s + "?"; }
    );
    print("String result: " + result1);
    assert(result1 == "Hello!?", "Should process string correctly");

    // Int handler
    DataHandler<Int> intHandler = new DataHandler<Int>(
        (Int n) : Bool => { return n > 0; },
        (Int n) : Int => { return n * 2; }
    );

    Int result2 = await intHandler.process(
        5,
        (Int n) : Int => { return n + 10; }
    );
    print("Int result: " + result2.toString());
    assert(result2 == 20, "Should process int correctly"); // (5 * 2) + 10 = 20

    // Polymorphic usage
    String result3 = await processWithInterface<String>(
        strHandler,
        "Test",
        (String s) : String => { return "[" + s + "]"; }
    );
    print("Polymorphic result: " + result3);
    assert(result3 == "[Test!]", "Should work polymorphically");

    print("Polymorphic async test passed");
}
