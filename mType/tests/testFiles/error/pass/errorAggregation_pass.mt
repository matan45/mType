// Test: Exception aggregation - collecting multiple exceptions
// Expected: Should collect and report multiple exceptions together
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Custom exceptions
class ValidationException extends Exception {
    public string field;

    public constructor(string msg, string fieldName) {
        super(msg);
        field = fieldName;
    }

    public function getField(): string {
        return field;
    }
}

// Aggregate exception container
class AggregateException extends Exception {
    private Exception[] innerExceptions;
    private int count;

    public constructor(string msg, Exception[] exceptions) {
        super(msg);
        innerExceptions = exceptions;
        count = 0;

        // Count non-null exceptions
        int i = 0;
        while (i < 10) {  // Assuming max 10 exceptions
            if (i < exceptions.length && exceptions[i] != null) {
                count = count + 1;
            }
            i = i + 1;
        }
    }

    public function getInnerExceptions(): Exception[] {
        return innerExceptions;
    }

    public function getCount(): int {
        return count;
    }

    public function toString(): string {
        string result = "AggregateException: " + message + " (" + count + " errors)";
        int i = 0;
        while (i < count) {
            result = result + "\n  - " + innerExceptions[i].getMessage();
            i = i + 1;
        }
        return result;
    }
}

// Test 1: Validate form with multiple field errors
print("=== Test 1: Form validation with multiple errors ===");

class FormData {
    public string name;
    public string email;
    public int age;

    public constructor(string n, string e, int a) {
        name = n;
        email = e;
        age = a;
    }
}

function validateForm(FormData form): void {
    Exception[] errors = new Exception[10];
    int errorCount = 0;

    // Validate name
    if (form.name == "" || form.name == null) {
        errors[errorCount] = new ValidationException("Name is required", "name");
        errorCount = errorCount + 1;
        print("Validation error: Name is required");
    }

    // Validate email
    if (form.email == "" || form.email == null) {
        errors[errorCount] = new ValidationException("Email is required", "email");
        errorCount = errorCount + 1;
        print("Validation error: Email is required");
    } else if (!(form.email.contains("@"))) {
        errors[errorCount] = new ValidationException("Email must contain @", "email");
        errorCount = errorCount + 1;
        print("Validation error: Email must contain @");
    }

    // Validate age
    if (form.age < 0) {
        errors[errorCount] = new ValidationException("Age cannot be negative", "age");
        errorCount = errorCount + 1;
        print("Validation error: Age cannot be negative");
    } else if (form.age < 18) {
        errors[errorCount] = new ValidationException("Age must be 18 or older", "age");
        errorCount = errorCount + 1;
        print("Validation error: Age must be 18 or older");
    }

    // If there are errors, throw aggregate
    if (errorCount > 0) {
        throw new AggregateException("Form validation failed with " + errorCount + " errors", errors);
    }
}

// Test with invalid form
FormData invalidForm = new FormData("", "invalid-email", 15);
try {
    validateForm(invalidForm);
    print("Form validation passed");
} catch (AggregateException e) {
    print("\nCaught aggregate exception:");
    print(e.toString());
}

// Test with valid form
print("\n--- Valid form test ---");
FormData validForm = new FormData("John Doe", "john@example.com", 25);
try {
    validateForm(validForm);
    print("Form validation passed");
} catch (AggregateException e) {
    print("Unexpected validation failure");
}

// Test 2: Parallel operation error aggregation
print("\n=== Test 2: Parallel operation errors ===");

class Task {
    public int id;
    public string name;
    public bool shouldFail;

    public constructor(int taskId, string taskName, bool fail) {
        id = taskId;
        name = taskName;
        shouldFail = fail;
    }

    public function execute(): string {
        if (shouldFail) {
            throw new Exception("Task " + id + " (" + name + ") failed");
        }
        return "Task " + id + " completed";
    }
}

Task[] tasks = [
    new Task(1, "Init", false),
    new Task(2, "Load", true),
    new Task(3, "Process", true),
    new Task(4, "Save", false),
    new Task(5, "Cleanup", true)
];

Exception[] taskErrors = new Exception[10];
int taskErrorCount = 0;
int taskSuccessCount = 0;

int i = 0;
while (i < 5) {
    try {
        string result = tasks[i].execute();
        taskSuccessCount = taskSuccessCount + 1;
        print("Success: " + result);
    } catch (Exception e) {
        taskErrors[taskErrorCount] = e;
        taskErrorCount = taskErrorCount + 1;
        print("Failed: " + e.getMessage());
    }
    i = i + 1;
}

print("\nTask execution summary:");
print("Successful: " + taskSuccessCount);
print("Failed: " + taskErrorCount);

if (taskErrorCount > 0) {
    AggregateException taskException = new AggregateException(
        "Batch execution completed with errors",
        taskErrors
    );
    print("\n" + taskException.toString());
}

// Test 3: Nested aggregate exceptions
print("\n=== Test 3: Nested aggregation ===");

function operationGroup1(): void {
    Exception[] errors = new Exception[5];
    errors[0] = new Exception("Group1-Error1");
    errors[1] = new Exception("Group1-Error2");
    throw new AggregateException("Group 1 failed", errors);
}

function operationGroup2(): void {
    Exception[] errors = new Exception[5];
    errors[0] = new Exception("Group2-Error1");
    throw new AggregateException("Group 2 failed", errors);
}

Exception[] groupErrors = new Exception[10];
int groupErrorCount = 0;

try {
    operationGroup1();
} catch (AggregateException e) {
    print("Caught from group 1: " + e.getMessage());
    groupErrors[groupErrorCount] = e;
    groupErrorCount = groupErrorCount + 1;
}

try {
    operationGroup2();
} catch (AggregateException e) {
    print("Caught from group 2: " + e.getMessage());
    groupErrors[groupErrorCount] = e;
    groupErrorCount = groupErrorCount + 1;
}

AggregateException topLevel = new AggregateException(
    "Multiple operation groups failed",
    groupErrors
);

print("\nNested aggregate exception:");
print(topLevel.toString());

// Test 4: Error collection with continuation
print("\n=== Test 4: Collect all errors before reporting ===");

class Validator {
    private Exception[] errors;
    private int errorCount;

    public constructor() {
        errors = new Exception[10];
        errorCount = 0;
    }

    public function addError(string message, string field): void {
        errors[errorCount] = new ValidationException(message, field);
        errorCount = errorCount + 1;
    }

    public function hasErrors(): bool {
        return errorCount > 0;
    }

    public function throwIfErrors(): void {
        if (hasErrors()) {
            throw new AggregateException("Validation failed", errors);
        }
    }

    public function getErrorCount(): int {
        return errorCount;
    }
}

Validator validator = new Validator();

// Collect multiple validation errors
print("Running validations...");
if (true) {
    validator.addError("Field1 is invalid", "field1");
    print("Added error for field1");
}

if (true) {
    validator.addError("Field2 is too short", "field2");
    print("Added error for field2");
}

if (true) {
    validator.addError("Field3 format is incorrect", "field3");
    print("Added error for field3");
}

print("Total errors collected: " + validator.getErrorCount());

// Throw all at once
try {
    validator.throwIfErrors();
} catch (AggregateException e) {
    print("\nAll validation errors:");
    print(e.toString());
}

// Test 5: Flattening nested exceptions
print("\n=== Test 5: Exception flattening ===");

function flattenExceptions(AggregateException ae): Exception[] {
    Exception[] flat = new Exception[20];
    int flatCount = 0;

    Exception[] inner = ae.getInnerExceptions();
    int i = 0;
    while (i < ae.getCount()) {
        Exception current = inner[i];

        // Check if it's another AggregateException (simple check)
        if (current.getMessage().contains("failed")) {
            print("Found nested aggregate: " + current.getMessage());
            flat[flatCount] = current;
            flatCount = flatCount + 1;
        } else {
            flat[flatCount] = current;
            flatCount = flatCount + 1;
        }

        i = i + 1;
    }

    return flat;
}

Exception[] mixedErrors = new Exception[5];
mixedErrors[0] = new Exception("Simple error 1");
mixedErrors[1] = new Exception("Simple error 2");

AggregateException mixed = new AggregateException("Mixed errors", mixedErrors);
Exception[] flattened = flattenExceptions(mixed);

print("Flattened exception list:");
int j = 0;
while (j < 2) {
    if (flattened[j] != null) {
        print("  " + (j + 1) + ". " + flattened[j].getMessage());
    }
    j = j + 1;
}

print("\nException aggregation test completed!");
