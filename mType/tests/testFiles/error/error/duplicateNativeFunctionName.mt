// Error: Duplicate native function name declaration

native function readFile(string path): string;

// ERROR: Duplicate function with same name
native function readFile(string filename): string;

print("This should not execute");
