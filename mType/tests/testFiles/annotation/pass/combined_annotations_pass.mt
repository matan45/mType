// Test: @Throw annotation combined with @Override
// Expected: Should compile and validate successfully
import * from "../../lib/exceptions/Exception.mt";
class IOException extends Exception {
    constructor(string message): super(message) {
    }
}

class FileReader {
    @Throw(IOException)
    public function read(string path): string {
        return "default content";
    }
	
	@Throw(IOException)
    public static function read2(string path): string {
        return "default content";
    }
}

class BufferedFileReader extends FileReader {
    @Override
    @Throw(IOException)
    public function read(string path): string {
        // Override with same exception declaration
        return "buffered content";
    }
}

// Test execution
BufferedFileReader reader = new BufferedFileReader();
string content = reader.read("test.txt");
print("Content: " + content);
