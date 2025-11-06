// Test: Realistic scenario using all features integration
// @Script
// Simulates a data processing pipeline with async operations, error handling,
// generics, casting, and control flow

interface Validator<T> {
    function validate(T item) : Bool;
}

interface Transformer<T, U> {
    function transform(T item) : U;
}

interface AsyncLoader<T> {
    function async load() : Promise<T>;
}

class DataRecord {
    public Int id;
    public String content;
    public Int timestamp;
    public Bool isValid;

    constructor(Int i, String c, Int t) {
        this.id = i;
        this.content = c;
        this.timestamp = t;
        this.isValid = false;
    }

    function toString() : String {
        return "Record(" + this.id.toString() + ", " + this.content + ", " + this.timestamp.toString() + ")";
    }
}

class ProcessedRecord extends DataRecord {
    private String processedContent;
    private Int processingTime;

    constructor(Int i, String c, Int t, String pc, Int pt) {
        super(i, c, t);
        this.processedContent = pc;
        this.processingTime = pt;
    }

    function getProcessedContent() : String {
        return this.processedContent;
    }

    function getProcessingTime() : Int {
        return this.processingTime;
    }

    function toString() : String {
        return "ProcessedRecord(" + this.id.toString() + ", " + this.processedContent + ", " + this.processingTime.toString() + "ms)";
    }
}

class DataValidator<T extends DataRecord> implements Validator<T> {
    private Int minContentLength;
    private Int maxTimestamp;

    constructor(Int minLen, Int maxTime) {
        this.minContentLength = minLen;
        this.maxTimestamp = maxTime;
    }

    function validate(T item) : Bool {
        try {
            if (item.content.length() < this.minContentLength) {
                throw "Content too short: " + item.content.length().toString();
            }

            if (item.timestamp > this.maxTimestamp) {
                throw "Timestamp too new: " + item.timestamp.toString();
            }

            item.isValid = true;
            return true;

        } catch (String e) {
            print("Validation failed for record " + item.id.toString() + ": " + e);
            return false;
        }
    }
}

class RecordTransformer implements Transformer<DataRecord, ProcessedRecord> {
    private Int processingDelay;

    constructor(Int delay) {
        this.processingDelay = delay;
    }

    function transform(DataRecord item) : ProcessedRecord {
        if (!item.isValid) {
            throw "Cannot transform invalid record";
        }

        String processed = item.content.toUpperCase();
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
    private Int recordCount;
    private Int failureRate;

    constructor(Int count, Int failRate) {
        this.recordCount = count;
        this.failureRate = failRate;
    }

    function async load() : Promise<DataRecord[]> {
        await delay(10);

        print("Loading " + this.recordCount.toString() + " records...");

        DataRecord[] records = new DataRecord[this.recordCount];
        Int i = 0;

        while (i < this.recordCount) {
            try {
                // Simulate occasional failures
                if (i % this.failureRate == 0 && i > 0) {
                    throw "Failed to load record " + i.toString();
                }

                String content = "Data_" + i.toString();
                Int timestamp = 1000 + i * 100;
                records[i] = new DataRecord(i, content, timestamp);

                await delay(5);

            } catch (String e) {
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
    private Validator<T> validator;
    private Transformer<DataRecord, ProcessedRecord> transformer;
    private Int errorCount;
    private Int successCount;

    constructor(Validator<T> v, Transformer<DataRecord, ProcessedRecord> t) {
        this.validator = v;
        this.transformer = t;
        this.errorCount = 0;
        this.successCount = 0;
    }

    function async processRecords(T[] records) : Promise<ProcessedRecord[]> {
        ProcessedRecord[] results = new ProcessedRecord[records.length()];
        Int resultIndex = 0;
        Int i = 0;

        print("\n=== Processing Pipeline Started ===");

        while (i < records.length()) {
            try {
                T record = records[i];

                // Stage 1: Validation
                print("Validating record " + record.id.toString() + "...");
                Bool isValid = this.validator.validate(record);

                if (!isValid) {
                    this.errorCount = this.errorCount + 1;
                    i = i + 1;
                    continue;
                }

                // Stage 2: Transformation
                print("Transforming record " + record.id.toString() + "...");

                // Cast to base type for transformer
                DataRecord baseRecord = record as DataRecord;
                ProcessedRecord processed = this.transformer.transform(baseRecord);

                results[resultIndex] = processed;
                resultIndex = resultIndex + 1;
                this.successCount = this.successCount + 1;

                print("Successfully processed: " + processed.toString());

            } catch (String e) {
                print("Pipeline error at record " + i.toString() + ": " + e);
                this.errorCount = this.errorCount + 1;
            }

            await delay(5);
            i = i + 1;
        }

        print("=== Pipeline Completed ===");
        print("Success: " + this.successCount.toString() + ", Errors: " + this.errorCount.toString());

        // Resize results to actual size
        ProcessedRecord[] finalResults = new ProcessedRecord[resultIndex];
        Int j = 0;
        while (j < resultIndex) {
            finalResults[j] = results[j];
            j = j + 1;
        }

        return finalResults;
    }

    function getStatistics() : String {
        return "Pipeline Stats - Success: " + this.successCount.toString() + ", Errors: " + this.errorCount.toString();
    }

    function resetStatistics() : void {
        this.errorCount = 0;
        this.successCount = 0;
    }
}

function async analyzeResults<T extends ProcessedRecord>(T[] results) : Promise<void> {
    print("\n=== Results Analysis ===");

    if (results.length() == 0) {
        print("No results to analyze");
        return;
    }

    Int totalTime = 0;
    Int maxTime = 0;
    Int minTime = results[0].getProcessingTime();
    Int i = 0;

    while (i < results.length()) {
        try {
            T record = results[i];

            // Cast to ProcessedRecord to access processing time
            ProcessedRecord processed = record as ProcessedRecord;
            Int time = processed.getProcessingTime();

            totalTime = totalTime + time;

            if (time > maxTime) {
                maxTime = time;
            }

            if (time < minTime) {
                minTime = time;
            }

        } catch (String e) {
            print("Analysis error at index " + i.toString() + ": " + e);
        }

        i = i + 1;
    }

    Int avgTime = totalTime / results.length();

    print("Total records: " + results.length().toString());
    print("Average processing time: " + avgTime.toString() + "ms");
    print("Min processing time: " + minTime.toString() + "ms");
    print("Max processing time: " + maxTime.toString() + "ms");
}

function async retryFailedRecords<T extends DataRecord>(
    T[] records,
    Validator<T> validator,
    Int maxRetries
) : Promise<Int> {
    print("\n=== Retry Failed Records ===");

    Int successCount = 0;
    Int i = 0;

    while (i < records.length()) {
        T record = records[i];

        if (!record.isValid) {
            print("Retrying record " + record.id.toString());

            Int attempt = 0;
            Bool success = false;

            while (attempt < maxRetries && !success) {
                try {
                    await delay(5);
                    success = validator.validate(record);

                    if (success) {
                        print("  Retry successful on attempt " + (attempt + 1).toString());
                        successCount = successCount + 1;
                    }

                } catch (String e) {
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

function async delay(Int ms) : Promise<void> {
    // Simulated delay
}

function async main() : Promise<void> {
    print("=== Real World Scenario Test ===");
    print("Simulating data processing pipeline...\n");

    // Step 1: Load data asynchronously
    print("--- Step 1: Data Loading ---");
    AsyncDataLoader loader = new AsyncDataLoader(8, 4);
    DataRecord[] records = await loader.load();

    // Step 2: Setup pipeline
    print("\n--- Step 2: Pipeline Setup ---");
    DataValidator<DataRecord> validator = new DataValidator<DataRecord>(5, 2000);
    RecordTransformer transformer = new RecordTransformer(50);
    ProcessingPipeline<DataRecord> pipeline = new ProcessingPipeline<DataRecord>(
        validator,
        transformer
    );

    // Step 3: Process records through pipeline
    print("\n--- Step 3: Pipeline Processing ---");
    ProcessedRecord[] processedRecords = await pipeline.processRecords(records);

    // Step 4: Analyze results
    print("\n--- Step 4: Results Analysis ---");
    await analyzeResults<ProcessedRecord>(processedRecords);

    // Step 5: Retry failed records
    print("\n--- Step 5: Retry Mechanism ---");
    Int retriedCount = await retryFailedRecords<DataRecord>(records, validator, 2);
    print("Successfully retried: " + retriedCount.toString() + " records");

    // Step 6: Final statistics
    print("\n--- Step 6: Final Statistics ---");
    print(pipeline.getStatistics());

    // Step 7: Type-safe casting and validation
    print("\n--- Step 7: Type Safety Checks ---");
    Int i = 0;
    while (i < processedRecords.length()) {
        try {
            ProcessedRecord record = processedRecords[i];

            // Cast to base type
            DataRecord base = record as DataRecord;
            print("Record " + base.id.toString() + " is valid: " + base.isValid.toString());

            // Try to cast back
            ProcessedRecord? processed = base as ProcessedRecord;
            if (processed != null) {
                print("  Successful downcast, content: " + processed.getProcessedContent());
            }

        } catch (String e) {
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
