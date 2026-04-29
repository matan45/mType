// Edge Case: Pattern matching inside async function body

import * from "../../lib/primitives/String.mt";

abstract class Result {
    abstract function getValue(): string;
}

class Success extends Result {
    private string data;
    public constructor(string data) { this.data = data; }
    public function getValue(): string { return this.data; }
}

class Failure extends Result {
    private string error;
    public constructor(string error) { this.error = error; }
    public function getValue(): string { return this.error; }
    public function getError(): string { return this.error; }
}

class Pending extends Result {
    public constructor() {}
    public function getValue(): string { return "pending"; }
}

function async processResult(Result r): Promise<String> {
    string output = "";
    match (r) {
        case Success s -> {
            output = "OK: " + s.getValue();
        }
        case Failure f -> {
            output = "ERROR: " + f.getError();
        }
        case Pending p -> {
            output = "WAITING";
        }
        default -> {
            output = "UNKNOWN";
        }
    }
    return new String(output);
}

function async handleResults(Result[] results): Promise<void> {
    for (int i = 0; i < results.length; i++) {
        String msg = await processResult(results[i]);
        print($"Result {i}: {msg.getValue()}");
    }
}

function async main(): Promise<void> {
    print("=== Edge: Match Inside Async ===");

    Result[] results = [
        new Success("data loaded"),
        new Failure("timeout"),
        new Pending(),
        new Success("cached"),
        new Failure("404 not found")
    ];

    await handleResults(results);

    print("--- Match on await result ---");
    Result dynamicResult = new Success("dynamic");
    String processed = await processResult(dynamicResult);
    match (dynamicResult) {
        case Success s -> print("Was success: " + processed.getValue());
        case Failure f -> print("Was failure: " + processed.getValue());
        default -> print("Other: " + processed.getValue());
    }

    print("=== Edge Complete ===");
}

main();
