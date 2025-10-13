// Error: Duplicate function - regular and native with same name

function processData(string data): string {
    return data + " processed";
}

// ERROR: Native function with same name as regular function
native function processData(string input): string;

print("This should not execute");
