// Test: Class implementing multiple async interfaces
// @Script

interface AsyncValidator {
    async validate(input: String) : Promise<Bool>;
}

interface AsyncTransformer {
    async transform(input: String) : Promise<String>;
}

interface AsyncLogger {
    async log(message: String) : Promise<Void>;
}

class DataProcessor implements AsyncValidator, AsyncTransformer, AsyncLogger {
    private logs: String[];

    constructor() {
        this.logs = [];
    }

    async validate(input: String) : Promise<Bool> {
        await delay(5);
        await this.log("Validating: " + input);
        return input.length() > 0;
    }

    async transform(input: String) : Promise<String> {
        await delay(5);
        await this.log("Transforming: " + input);
        return input + " [processed]";
    }

    async log(message: String) : Promise<Void> {
        await delay(5);
        this.logs.push(message);
        print("LOG: " + message);
    }

    getLogCount() : Int {
        return this.logs.length();
    }
}

async delay(ms: Int) : Promise<Void> {
    // Simulated delay
}

async processData(
    data: String,
    validator: AsyncValidator,
    transformer: AsyncTransformer
) : Promise<String> {
    let isValid = await validator.validate(data);
    if (isValid) {
        return await transformer.transform(data);
    }
    return "Invalid";
}

async main() : Promise<Void> {
    let processor = new DataProcessor();

    // Test as validator
    let valid: AsyncValidator = processor;
    let isValid = await valid.validate("test");
    assert(isValid, "Should validate non-empty string");

    // Test as transformer
    let transformer: AsyncTransformer = processor;
    let transformed = await transformer.transform("data");
    assert(transformed == "data [processed]", "Should transform correctly");

    // Test combined
    let result = await processData("input", processor, processor);
    print("Result: " + result);
    assert(result == "input [processed]", "Should process through multiple interfaces");

    assert(processor.getLogCount() > 0, "Should have logged operations");
}
