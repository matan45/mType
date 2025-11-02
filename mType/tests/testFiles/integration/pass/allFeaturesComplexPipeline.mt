// Test: Complex data pipeline using all features
// @Script

import "modules/DataPipelineInterfaces.mt";

class NumberSource : AsyncSource<Int> {
    private numbers: Int[];

    constructor(nums: Int[]) {
        this.numbers = nums;
    }

    async fetch() : Promise<Int[]> {
        await delay(10);
        print("Fetching " + this.numbers.length().toString() + " numbers");
        return this.numbers;
    }
}

class StringProcessor : AsyncProcessor<Int, String> {
    private transform: (Int) -> String;

    constructor(t: (Int) -> String) {
        this.transform = t;
    }

    async process(data: Int) : Promise<String> {
        await delay(5);
        return this.transform(data);
    }
}

class StringSink : AsyncSink<String> {
    private storage: String[];

    constructor() {
        this.storage = [];
    }

    async write(data: String[]) : Promise<Int> {
        await delay(10);
        let i: Int = 0;
        while (i < data.length()) {
            this.storage.push(data[i]);
            i = i + 1;
        }
        print("Written " + data.length().toString() + " items");
        return data.length();
    }

    getData() : String[] {
        return this.storage;
    }
}

async runPipeline<T, R>(
    source: AsyncSource<T>,
    processor: AsyncProcessor<T, R>,
    sink: AsyncSink<R>,
    filter: (T) -> Bool
) : Promise<Int> {
    // Fetch data
    let data = await source.fetch();

    // Filter with lambda
    let filtered: T[] = [];
    let i: Int = 0;
    while (i < data.length()) {
        if (filter(data[i])) {
            filtered.push(data[i]);
        }
        i = i + 1;
    }

    // Process each item
    let processed: R[] = [];
    let j: Int = 0;
    while (j < filtered.length()) {
        let result = await processor.process(filtered[j]);
        processed.push(result);
        j = j + 1;
    }

    // Write results
    let count = await sink.write(processed);
    return count;
}

async delay(ms: Int) : Promise<Void> {
    // Simulated delay
}

async main() : Promise<Void> {
    let numbers: Int[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    let source = new NumberSource(numbers);

    let processor = new StringProcessor((n: Int) : String => {
        return "Value: " + (n * n).toString();
    });

    let sink = new StringSink();

    // Run pipeline with lambda filter (only even numbers)
    let count = await runPipeline<Int, String>(
        source,
        processor,
        sink,
        (n: Int) : Bool => { return n % 2 == 0; }
    );

    print("Processed count: " + count.toString());
    assert(count == 5, "Should process 5 even numbers");

    let data = sink.getData();
    print("First result: " + data[0]);
    assert(data[0] == "Value: 4", "Should square and format correctly"); // 2^2 = 4

    print("Complex pipeline test passed");
}
