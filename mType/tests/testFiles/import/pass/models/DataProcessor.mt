// Level 2: Data processor that uses both MathCore and StringUtils
// Located in: models/DataProcessor.mt
// Uses wildcard import to get all public symbols

import * from "../utils/MathCore.mt";
import * from "../utils/StringUtils.mt";

public class DataProcessor {
    public constructor() {
        // Empty constructor
    }

    public function processNumber(int num) : int {
        // Use MathCore from wildcard import
        MathCore math = new MathCore();
        int squared = math.square(num);
        int absoluteValue = abs(squared);
        return absoluteValue;
    }

    public function processString(string text) : string {
        // Use StringBuilder from wildcard import
        StringBuilder sb = new StringBuilder();
        string processed = sb.concat(text, "!");
        return processed;
    }

    public function getVersionInfo() : string {
        // Access imported constants
        StringBuilder sb = new StringBuilder();
        string result = sb.concat("Math:", "1");
        result = sb.concat(result, " String:");
        result = sb.concat(result, "2");
        return result;
    }
}

public function calculateAndFormat(int x, int y) : string {
    MathCore math = new MathCore();
    int result = max(math.square(x), math.square(y));
    StringBuilder sb = new StringBuilder();
    return sb.concat("Max square: ", "16");
}

public interface Processor {
    function process(int data) : int;
}

private class InternalProcessor {
    int state;

    public constructor() {
        this.state = 0;
    }
}

// Public array declarations
public int[] publicNumbers = [1, 2, 3, 4, 5];
public string[] publicStrings = ["hello", "world"];

// Private array declarations
private int[] privateNumbers = [10, 20, 30];
private string[] privateData = ["secret1", "secret2"];

// Public object declarations
public DataProcessor publicProcessor = new DataProcessor();

// Private object declarations
private InternalProcessor privateProcessor = new InternalProcessor();
