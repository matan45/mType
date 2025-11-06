// Test: Exception propagation through cast chains
// Expected: Pass - demonstrates exception flow through multiple cast levels

import * from "../../lib/exceptions/Exception.mt";

class ValidationException extends Exception {
    constructor(String msg):super(msg) {
    }
}

class ProcessingException extends Exception {
    constructor(String msg):super(msg) {
    }
}

class Entity {
    protected String id;

    public constructor(String id) {
        this.id = id;
    }

    public String getId() {
        return this.id;
    }

    public void validate() {
        print("Validating entity: " + this.id);
    }
}

class Document extends Entity {
    private String title;

    public constructor(String id, String title) : super(id) {
        this.title = title;
    }

    public String getTitle() {
        return this.title;
    }

    public void validate() {
        print("Validating document: " + this.title);
        if (this.title == "invalid") {
            throw new ValidationException("Invalid document title");
        }
    }

    public void process() {
        print("Processing document: " + this.title);
    }
}

class Report extends Document {
    private String type;

    public constructor(String id, String title, String type) : super(id, title) {
        this.type = type;
    }

    public String getType() {
        return this.type;
    }

    public void validate() {
        print("Validating report: " + this.getTitle() + " (" + this.type + ")");
        if (this.type == "error") {
            throw new ValidationException("Invalid report type");
        }
    }

    public void generate() {
        print("Generating report: " + this.getTitle());
    }
}

// Test 1: Exception propagates through single cast
String singleCastPropagation(Entity e) {
    try {
        if (e instanceof Document) {
            Document d = (Document)e;
            d.validate();
            d.process();
            return "Document processed";
        }
        return "Not a document";
    } catch (ValidationException e) {
        return "Validation failed: " + e.getMessage();
    }
}

// Test 2: Exception propagates through multiple cast levels
String multiLevelCastPropagation(Entity e) {
    try {
        if (e instanceof Document) {
            Document d = (Document)e;
            print("First level: Document cast successful");

            if (d instanceof Report) {
                Report r = (Report)d;
                print("Second level: Report cast successful");
                r.validate();
                r.generate();
                return "Report generated";
            } else {
                d.validate();
                d.process();
                return "Document processed";
            }
        }
        return "Not a document";
    } catch (ValidationException e) {
        return "Caught at top level: " + e.getMessage();
    }
}

// Test 3: Exception propagates through nested functions with casts
void validateEntity(Entity e) {
    if (e instanceof Document) {
        Document d = (Document)e;
        d.validate();
    }
}

void processEntity(Entity e) {
    if (e instanceof Document) {
        Document d = (Document)e;
        d.process();
    }
}

String nestedCastPropagation(Entity e) {
    try {
        validateEntity(e);
        processEntity(e);
        return "Entity pipeline complete";
    } catch (ValidationException e) {
        return "Pipeline failed: " + e.getMessage();
    }
}

// Test 4: Exception propagates through cast chain with re-throw
void innerCastCheck(Entity e) {
    if (e instanceof Report) {
        Report r = (Report)e;
        print("Inner: Validating report");
        r.validate();
    } else {
        throw new ProcessingException("Not a report");
    }
}

void middleCastCheck(Entity e) {
    try {
        if (e instanceof Document) {
            Document d = (Document)e;
            print("Middle: Cast to document");
            innerCastCheck(e);
        }
    } catch (ValidationException e) {
        print("Middle: Caught validation error");
        throw e;
    }
}

String outerCastPropagation(Entity e) {
    try {
        middleCastCheck(e);
        return "Chain completed";
    } catch (ValidationException e) {
        return "Outer caught: " + e.getMessage();
    } catch (ProcessingException e) {
        return "Processing error: " + e.getMessage();
    }
}

// Test 5: Mixed exception types in cast chain
String complexCastPropagation(Entity e1, Entity e2) {
    try {
        if (e1 instanceof Report) {
            Report r = (Report)e1;
            r.validate();
            print("First report validated");
        }

        if (e2 instanceof Report) {
            Report r = (Report)e2;
            r.validate();
            print("Second report validated");
        }

        return "Both validated";
    } catch (ValidationException e) {
        print("Caught validation exception: " + e.getMessage());
        throw new ProcessingException("Validation failed in complex chain");
    }
}

String wrapperCastPropagation(Entity e1, Entity e2) {
    try {
        return complexCastPropagation(e1, e2);
    } catch (ProcessingException e) {
        return "Wrapper caught: " + e.getMessage();
    }
}

// Test 6: Array processing with exception propagation
String processEntityArray(Entity[] entities) {
    Int processed = 0;

    try {
        Int i = 0;
        while (i < 3) {
            if (entities[i] instanceof Document) {
                Document d = (Document)entities[i];
                d.validate();
                d.process();
                processed = processed + 1;
            }
            i = i + 1;
        }
        return "Processed: " + processed;
    } catch (ValidationException e) {
        return "Failed at item " + processed + ": " + e.getMessage();
    }
}

// Run all tests
print("=== Test 1: Single cast propagation (valid) ===");
Entity doc1 = new Document("doc1", "Valid Document");
print(singleCastPropagation(doc1));

print("\n=== Test 1b: Single cast propagation (invalid) ===");
Entity doc2 = new Document("doc2", "invalid");
print(singleCastPropagation(doc2));

print("\n=== Test 2: Multi-level cast propagation (valid) ===");
Entity report1 = new Report("rep1", "Annual Report", "financial");
print(multiLevelCastPropagation(report1));

print("\n=== Test 2b: Multi-level cast propagation (invalid) ===");
Entity report2 = new Report("rep2", "Error Report", "error");
print(multiLevelCastPropagation(report2));

print("\n=== Test 3: Nested function propagation (valid) ===");
Entity doc3 = new Document("doc3", "Working Doc");
print(nestedCastPropagation(doc3));

print("\n=== Test 3b: Nested function propagation (invalid) ===");
Entity doc4 = new Document("doc4", "invalid");
print(nestedCastPropagation(doc4));

print("\n=== Test 4: Cast chain with re-throw (valid) ===");
Entity report3 = new Report("rep3", "Status Report", "status");
print(outerCastPropagation(report3));

print("\n=== Test 4b: Cast chain with re-throw (invalid) ===");
Entity report4 = new Report("rep4", "Failure Report", "error");
print(outerCastPropagation(report4));

print("\n=== Test 4c: Cast chain with wrong type ===");
Entity doc5 = new Document("doc5", "Plain Document");
print(outerCastPropagation(doc5));

print("\n=== Test 5: Mixed exception types (valid) ===");
Entity report5 = new Report("rep5", "Report A", "audit");
Entity report6 = new Report("rep6", "Report B", "summary");
print(wrapperCastPropagation(report5, report6));

print("\n=== Test 5b: Mixed exception types (invalid) ===");
Entity report7 = new Report("rep7", "Report C", "error");
Entity report8 = new Report("rep8", "Report D", "normal");
print(wrapperCastPropagation(report7, report8));

print("\n=== Test 6: Array processing (partial failure) ===");
Entity[] arr = new Entity[3];
arr[0] = new Document("doc6", "Doc A");
arr[1] = new Document("doc7", "invalid");
arr[2] = new Document("doc8", "Doc C");
print(processEntityArray(arr));

print("\nAll exception propagation tests completed");
