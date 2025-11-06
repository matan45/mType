// Test: Complex data pipeline using all features
// @Script

import "modules/DataPipelineInterfaces.mt";

class NumberSource implements AsyncSource<Int> {
    private Int[] numbers;

    constructor(Int[] nums) {
        this.numbers = nums;
    }

    function async fetch() : Promise<Int[]> {
        await delay(10);
        print("Fetching " + this.numbers.length().toString() + " numbers");
        return this.numbers;
    }
}

class StringProcessor implements AsyncProcessor<Int, String> {
    private (Int) -> String transform;

    constructor((Int) -> String t) {
        this.transform = t;
    }

    function async process(Int data) : Promise<String> {
        await delay(5);
        return this.transform(data);
    }
}

class StringSink implements AsyncSink<String> {
    private String[] storage;

    constructor() {
        this.storage = [];
    }

    function async write(String[] data) : Promise<Int> {
        await delay(10);
        Int i = 0;
        while (i < data.length()) {
            this.storage.push(data[i]);
            i = i + 1;
        }
        print("Written " + data.length().toString() + " items");
        return data.length();
    }

    function getData() : String[] {
        return this.storage;
    }
}

function async runPipeline<T, R>(
    AsyncSource<T> source,
    AsyncProcessor<T, R> processor,
    AsyncSink<R> sink,
    (T) -> Bool filter
) : Promise<Int> {
    // Fetch data
    T[] data = await source.fetch();

    // Filter with lambda
    T[] filtered = [];
    Int i = 0;
    while (i < data.length()) {
        if (filter(data[i])) {
            filtered.push(data[i]);
        }
        i = i + 1;
    }

    // Process each item
    R[] processed = [];
    Int j = 0;
    while (j < filtered.length()) {
        R result = await processor.process(filtered[j]);
        processed.push(result);
        j = j + 1;
    }

    // Write results
    Int count = await sink.write(processed);
    return count;
}

function async delay(Int ms) : Promise<void> {
    // Simulated delay
}

function async main() : Promise<void> {
    Int[] numbers = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    NumberSource source = new NumberSource(numbers);

    StringProcessor processor = new StringProcessor((Int n) : String => {
        return "Value: " + (n * n).toString();
    });

    StringSink sink = new StringSink();

    // Run pipeline with lambda filter (only even numbers)
    Int count = await runPipeline<Int, String>(
        source,
        processor,
        sink,
        (Int n) : Bool => { return n % 2 == 0; }
    );

    print("Processed count: " + count.toString());
    assert(count == 5, "Should process 5 even numbers");

    String[] data = sink.getData();
    print("First result: " + data[0]);
    assert(data[0] == "Value: 4", "Should square and format correctly"); // 2^2 = 4

    print("Complex pipeline test passed");
}
