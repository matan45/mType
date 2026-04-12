// Test: Async/Await in standalone exe
import * from "../../../../lib/primitives/Int.mt";

function async computeSquare(int n): Promise<Int> {
    int result = n * n;
    return new Int(result);
}

function async sumSquares(int limit): Promise<Int> {
    int total = 0;
    for (int i = 1; i <= limit; i = i + 1) {
        Int val = await computeSquare(i);
        total = total + val.getValue();
    }
    return new Int(total);
}

function async runAsync(): Promise<Int> {
    print("Starting async computation");

    Int result = await sumSquares(4);
    print("Sum of squares 1..4 = " + result.getValue());

    Int single = await computeSquare(7);
    print("7 squared = " + single.getValue());

    return result;
}

@EntryPoint
class App {
    public static function main(string[] args): void {
        Int result = await runAsync();
        print("Final result: " + result.getValue());
        print("Async test passed");
    }
}
