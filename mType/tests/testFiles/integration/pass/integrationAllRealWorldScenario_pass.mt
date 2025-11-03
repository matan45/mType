// Test: Realistic scenario using all features integration
// @Script
// Simulates a data processing pipeline with async operations, error handling,
// generics, casting, and control flow

interface Validator<T> {
    validate(item: T) : Bool;
}

interface Transformer<T, U> {
    transform(item: T) : U;
}

interface AsyncLoader<T> {
    async load() : Promise<T>;
}

class DataRecord {
    public id: Int;
    public content: String;
    public timestamp: Int;
    public isValid: Bool;

    constructor(i: Int, c: String, t: Int) {
        this.id = i;
        this.content = c;
        this.timestamp = t;
        this.isValid = false;
    }

    toString() : String {
        return "Record(" + this.id.toString() + ", " + this.content + ", " + this.timestamp.toString() + ")";
    }
}

class ProcessedRecord extends DataRecord {
    private processedContent: String;
    private processingTime: Int;

    constructor(i: Int, c: String, t: Int, pc: String, pt: Int) {
        super(i, c, t);
        this.processedContent = pc;
        this.processingTime = pt;
    }

    getProcessedContent() : String {
        return this.processedContent;
    }

    getProcessingTime() : Int {
        return this.processingTime;
    }

    toString() : String {
        return "ProcessedRecord(" + this.id.toString() + ", " + this.processedContent + ", " + this.processingTime.toString() + "ms)";
    }
}

class DataValidator<T extends DataRecord> implements Validator<T> {
    private minContentLength: Int;
    private maxTimestamp: Int;

    constructor(minLen: Int, maxTime: Int) {
        this.minContentLength = minLen;
        this.maxTimestamp = maxTime;
    }

    validate(item: T) : Bool {
        try {
            if (item.content.length() < this.minContentLength) {
                throw "Content too short: " + item.content.length().toString();
            }

            if (item.timestamp > this.maxTimestamp) {
                throw "Timestamp too new: " + item.timestamp.toString();
            }

            item.isValid = true;
            return true;

        } catch (e: String) {
            print("Validation failed for record " + item.id.toString() + ": " + e);
            return false;
        }
    }
}

class RecordTransformer implements Transformer<DataRecord, ProcessedRecord> {
    private processingDelay: Int;

    constructor(delay: Int) {
        this.processingDelay = delay;
    }

    transform(item: DataRecord) : ProcessedRecord {
        if (!item.isValid) {
            throw "Cannot transform invalid record";
        }

        let processed: String = item.content.toUpperCase();
        return new ProcessedRecord(
            item.id,
            item.content,
            item.timestamp,
            processed,
            this.processingDelay
        );
    }
}

class AsyncDataLoader implements AsyncLoader<DataRecord[]> {
    private recordCount: Int;
    private failureRate: Int;

    constructor(count: Int, failRate: Int) {
        this.recordCount = count;
        this.failureRate = failRate;
    }

    async load() : Promise<DataRecord[]> {
        await delay(10);

        print("Loading " + this.recordCount.toString() + " records...");

        let records: DataRecord[] = new DataRecord[this.recordCount];
        let i: Int = 0;

        while (i < this.recordCount) {
            try {
                // Simulate occasional failures
                if (i % this.failureRate == 0 && i > 0) {
                    throw "Failed to load record " + i.toString();
                }

                let content: String = "Data_" + i.toString();
                let timestamp: Int = 1000 + i * 100;
                records[i] = new DataRecord(i, content, timestamp);

                await delay(5);

            } catch (e: String) {
                print("Load error: " + e);
                // Create placeholder record
                records[i] = new DataRecord(i, "", 0);
            }

            i = i + 1;
        }

        print("Loaded " + this.recordCount.toString() + " records");
        return records;
    }
}

class ProcessingPipeline<T extends DataRecord> {
    private validator: Validator<T>;
    private transformer: Transformer<DataRecord, ProcessedRecord>;
    private errorCount: Int;
    private successCount: Int;

    constructor(v: Validator<T>, t: Transformer<DataRecord, ProcessedRecord>) {
        this.validator = v;
        this.transformer = t;
        this.errorCount = 0;
        this.successCount = 0;
    }

    async processRecords(records: T[]) : Promise<ProcessedRecord[]> {
        let results: ProcessedRecord[] = new ProcessedRecord[records.length()];
        let resultIndex: Int = 0;
        let i: Int = 0;

        print("\n=== Processing Pipeline Started ===");

        while (i < records.length()) {
            try {
                let record: T = records[i];

                // Stage 1: Validation
                print("Validating record " + record.id.toString() + "...");
                let isValid: Bool = this.validator.validate(record);

                if (!isValid) {
                    this.errorCount = this.errorCount + 1;
                    i = i + 1;
                    continue;
                }

                // Stage 2: Transformation
                print("Transforming record " + record.id.toString() + "...");

                // Cast to base type for transformer
                let baseRecord: DataRecord = record as DataRecord;
                let processed: ProcessedRecord = this.transformer.transform(baseRecord);

                results[resultIndex] = processed;
                resultIndex = resultIndex + 1;
                this.successCount = this.successCount + 1;

                print("Successfully processed: " + processed.toString());

            } catch (e: String) {
                print("Pipeline error at record " + i.toString() + ": " + e);
                this.errorCount = this.errorCount + 1;
            }

            await delay(5);
            i = i + 1;
        }

        print("=== Pipeline Completed ===");
        print("Success: " + this.successCount.toString() + ", Errors: " + this.errorCount.toString());

        // Resize results to actual size
        let finalResults: ProcessedRecord[] = new ProcessedRecord[resultIndex];
        let j: Int = 0;
        while (j < resultIndex) {
            finalResults[j] = results[j];
            j = j + 1;
        }

        return finalResults;
    }

    getStatistics() : String {
        return "Pipeline Stats - Success: " + this.successCount.toString() + ", Errors: " + this.errorCount.toString();
    }

    resetStatistics() : Void {
        this.errorCount = 0;
        this.successCount = 0;
    }
}

async analyzeResults<T extends ProcessedRecord>(results: T[]) : Promise<Void> {
    print("\n=== Results Analysis ===");

    if (results.length() == 0) {
        print("No results to analyze");
        return;
    }

    let totalTime: Int = 0;
    let maxTime: Int = 0;
    let minTime: Int = results[0].getProcessingTime();
    let i: Int = 0;

    while (i < results.length()) {
        try {
            let record: T = results[i];

            // Cast to ProcessedRecord to access processing time
            let processed: ProcessedRecord = record as ProcessedRecord;
            let time: Int = processed.getProcessingTime();

            totalTime = totalTime + time;

            if (time > maxTime) {
                maxTime = time;
            }

            if (time < minTime) {
                minTime = time;
            }

        } catch (e: String) {
            print("Analysis error at index " + i.toString() + ": " + e);
        }

        i = i + 1;
    }

    let avgTime: Int = totalTime / results.length();

    print("Total records: " + results.length().toString());
    print("Average processing time: " + avgTime.toString() + "ms");
    print("Min processing time: " + minTime.toString() + "ms");
    print("Max processing time: " + maxTime.toString() + "ms");
}

async retryFailedRecords<T extends DataRecord>(
    records: T[],
    validator: Validator<T>,
    maxRetries: Int
) : Promise<Int> {
    print("\n=== Retry Failed Records ===");

    let successCount: Int = 0;
    let i: Int = 0;

    while (i < records.length()) {
        let record: T = records[i];

        if (!record.isValid) {
            print("Retrying record " + record.id.toString());

            let attempt: Int = 0;
            let success: Bool = false;

            while (attempt < maxRetries && !success) {
                try {
                    await delay(5);
                    success = validator.validate(record);

                    if (success) {
                        print("  Retry successful on attempt " + (attempt + 1).toString());
                        successCount = successCount + 1;
                    }

                } catch (e: String) {
                    print("  Retry attempt " + (attempt + 1).toString() + " failed: " + e);
                }

                attempt = attempt + 1;
            }

            if (!success) {
                print("  All retry attempts exhausted for record " + record.id.toString());
            }
        }

        i = i + 1;
    }

    return successCount;
}

async delay(ms: Int) : Promise<Void> {
    // Simulated delay
}

async main() : Promise<Void> {
    print("=== Real World Scenario Test ===");
    print("Simulating data processing pipeline...\n");

    // Step 1: Load data asynchronously
    print("--- Step 1: Data Loading ---");
    let loader: AsyncDataLoader = new AsyncDataLoader(8, 4);
    let records: DataRecord[] = await loader.load();

    // Step 2: Setup pipeline
    print("\n--- Step 2: Pipeline Setup ---");
    let validator: DataValidator<DataRecord> = new DataValidator<DataRecord>(5, 2000);
    let transformer: RecordTransformer = new RecordTransformer(50);
    let pipeline: ProcessingPipeline<DataRecord> = new ProcessingPipeline<DataRecord>(
        validator,
        transformer
    );

    // Step 3: Process records through pipeline
    print("\n--- Step 3: Pipeline Processing ---");
    let processedRecords: ProcessedRecord[] = await pipeline.processRecords(records);

    // Step 4: Analyze results
    print("\n--- Step 4: Results Analysis ---");
    await analyzeResults<ProcessedRecord>(processedRecords);

    // Step 5: Retry failed records
    print("\n--- Step 5: Retry Mechanism ---");
    let retriedCount: Int = await retryFailedRecords<DataRecord>(records, validator, 2);
    print("Successfully retried: " + retriedCount.toString() + " records");

    // Step 6: Final statistics
    print("\n--- Step 6: Final Statistics ---");
    print(pipeline.getStatistics());

    // Step 7: Type-safe casting and validation
    print("\n--- Step 7: Type Safety Checks ---");
    let i: Int = 0;
    while (i < processedRecords.length()) {
        try {
            let record: ProcessedRecord = processedRecords[i];

            // Cast to base type
            let base: DataRecord = record as DataRecord;
            print("Record " + base.id.toString() + " is valid: " + base.isValid.toString());

            // Try to cast back
            let processed: ProcessedRecord? = base as ProcessedRecord;
            if (processed != null) {
                print("  Successful downcast, content: " + processed.getProcessedContent());
            }

        } catch (e: String) {
            print("Type checking error: " + e);
        }

        i = i + 1;

        if (i >= 3) {
            print("  ... (showing first 3 records)");
            break;
        }
    }

    print("\n=== All tests completed ===");
}
