// Level 3: Main application using wildcard nested imports
// Located in: nested/test_wildcard_nested_imports.mt
// Imports from: models/DataProcessor.mt (which wildcard imports from utils)

import { DataProcessor, calculateAndFormat, Processor } from "../models/DataProcessor.mt";

function main() : void {
    // Test DataProcessor that uses both MathCore and StringUtils
    DataProcessor dp = new DataProcessor();

    int processedNum = dp.processNumber(5);
    print("Processed number (5^2): " + processedNum);

    string processedStr = dp.processString("Hello");
    print("Processed string: " + processedStr);

    string versionInfo = dp.getVersionInfo();
    print("Version info: " + versionInfo);

    // Test function that uses wildcard-imported symbols
    string formatted = calculateAndFormat(3, 4);
    print(formatted);

    print("Wildcard nested import test completed!");
}

main();
