// Test: Class implementing multiple async interfaces
// @Script

interface AsyncValidator {
    function async validate(String input) : Promise<Bool>;
}

interface AsyncTransformer {
    function async transform(String input) : Promise<String>;
}

interface AsyncLogger {
    function async log(String message) : Promise<void>;
}

class DataProcessor implements AsyncValidator, AsyncTransformer, AsyncLogger {
    private String[] logs;

    constructor() {
        this.logs = [];
    }

    public function async validate(String input) : Promise<Bool> {
        await delay(5);
        await this.log("Validating: " + input);
        return input.length() > 0;
    }

    public function async transform(String input) : Promise<String> {
        await delay(5);
        await this.log("Transforming: " + input);
        return input + " [processed]";
    }

    public function async log(String message) : Promise<void> {
        await delay(5);
        this.logs.push(message);
        print("LOG: " + message);
    }

    public function getLogCount() : Int {
        return this.logs.length();
    }
}

function async delay(Int ms) : Promise<void> {
    // Simulated delay
}

function async processData(
    String data,
    AsyncValidator validator,
    AsyncTransformer transformer
) : Promise<String> {
    Bool isValid = await validator.validate(data);
    if (isValid) {
        return await transformer.transform(data);
    }
    return "Invalid";
}

function async main() : Promise<void> {
    DataProcessor processor = new DataProcessor();

    // Test as validator
    AsyncValidator valid = processor;
    Bool isValid = await valid.validate("test");
    assert(isValid, "Should validate non-empty string");

    // Test as transformer
    AsyncTransformer transformer = processor;
    String transformed = await transformer.transform("data");
    assert(transformed == "data [processed]", "Should transform correctly");

    // Test combined
    String result = await processData("input", processor, processor);
    print("Result: " + result);
    assert(result == "input [processed]", "Should process through multiple interfaces");

    assert(processor.getLogCount() > 0, "Should have logged operations");
}
