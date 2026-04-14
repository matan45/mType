// Test: @Throw annotation with non-existent exception class
// Expected: Should fail with "Exception class 'NonExistentException' does not exist"

@Throw(exceptions = [NonExistentException])
function readFile(string path): string {
    return "file contents";
}

// This should never execute due to compilation error
string result = readFile("test.txt");
