// Test empty string edge cases
// This tests various scenarios involving empty strings to ensure proper handling

// Test 1: Basic empty string operations
print("Testing basic empty string operations:");

string empty = "";
string notEmpty = "content";

// Test empty string initialization
print("Empty string length: " + toString(strlen(empty)));
print("Empty string is empty: " + toString(empty == ""));
print("Empty string is not null: " + toString(empty != null));

// Test 2: Empty string concatenation
print("\nTesting empty string concatenation:");

string result1 = empty + empty;
print("Empty + Empty = '" + result1 + "' (length: " + toString(strlen(result1)) + ")");

string result2 = empty + notEmpty;
print("Empty + NotEmpty = '" + result2 + "'");

string result3 = notEmpty + empty;
print("NotEmpty + Empty = '" + result3 + "'");

string result4 = empty + "literal";
print("Empty + Literal = '" + result4 + "'");

string result5 = "literal" + empty;
print("Literal + Empty = '" + result5 + "'");

// Test 3: Empty string in compound assignment
print("\nTesting compound assignment with empty strings:");

string accumulated = "";
accumulated += "";
print("After += empty: '" + accumulated + "' (length: " + toString(strlen(accumulated)) + ")");

accumulated += "first";
print("After += 'first': '" + accumulated + "'");

accumulated += "";
print("After += empty again: '" + accumulated + "'");

accumulated += "second";
print("After += 'second': '" + accumulated + "'");

// Test 4: Empty string comparisons
print("\nTesting empty string comparisons:");

bool comp1 = empty == "";
print("empty == '' : " + toString(comp1));

bool comp2 = empty == " ";  // Space is not empty
print("empty == ' ' : " + toString(comp2));

bool comp3 = empty == null;
print("empty == null : " + toString(comp3));

bool comp4 = empty != "";
print("empty != '' : " + toString(comp4));

bool comp5 = empty != " ";
print("empty != ' ' : " + toString(comp5));

bool comp6 = empty != null;
print("empty != null : " + toString(comp6));

// Test 5: Empty string in functions
print("\nTesting empty strings in functions:");

function processString(string s): string {
    if (s == "") {
        return "Input was empty";
    }
    return "Input: " + s;
}

print(processString(""));
print(processString("not empty"));

function concatenateStrings(string a, string b): string {
    return a + b;
}

print("concatenate('', ''): '" + concatenateStrings("", "") + "'");
print("concatenate('a', ''): '" + concatenateStrings("a", "") + "'");
print("concatenate('', 'b'): '" + concatenateStrings("", "b") + "'");

// Test 6: Empty string with loops
print("\nTesting empty strings with loops:");

string loopResult = "";
for (int i = 0; i < 3; i++) {
    if (i == 1) {
        loopResult += "";  // Add empty string in middle
    } else {
        loopResult += toString(i);
    }
}
print("Loop result with empty: '" + loopResult + "'");

// Test 7: Empty string with strlen function
print("\nTesting strlen function with various strings:");

print("strlen(''): " + toString(strlen("")));
print("strlen('a'): " + toString(strlen("a")));
print("strlen('hello'): " + toString(strlen("hello")));
print("strlen(' '): " + toString(strlen(" ")));
print("strlen('\\t'): " + toString(strlen("\t")));
print("strlen('\\n'): " + toString(strlen("\n")));

// Test 8: Empty string in conditional expressions
print("\nTesting empty strings in conditional expressions:");

string conditionalResult = empty == "" ? "was empty" : "not empty";
print("Ternary with empty: " + conditionalResult);

string value = "";
string conditionalResult2 = strlen(value) == 0 ? "zero length" : "has content";
print("Length check: " + conditionalResult2);

// Test 9: Empty string with whitespace variations
print("\nTesting whitespace edge cases:");

string space = " ";
string tab = "\t";
string newline = "\n";
string multiSpace = "   ";
string mixed = " \t\n ";

print("Space == empty: " + toString(space == empty));
print("Tab == empty: " + toString(tab == empty));
print("Newline == empty: " + toString(newline == empty));
print("MultiSpace == empty: " + toString(multiSpace == empty));
print("Mixed whitespace == empty: " + toString(mixed == empty));

// Test 10: Empty string in class contexts
print("\nTesting empty strings in classes:");

class StringContainer {
    string value;
    
    constructor() {
        value = "";  // Initialize with empty string
    }
    
    constructor(string s) {
        value = s;
    }
    
    function getValue(): string {
        return value;
    }
    
    function setValue(string s): void {
        value = s;
    }
    
    function isEmpty(): bool {
        return value == "";
    }
    
    function append(string s): void {
        value += s;
    }
}

StringContainer container1 = new StringContainer();
print("Default container value: '" + container1.getValue() + "'");
print("Default container isEmpty: " + toString(container1.isEmpty()));

container1.append("");
print("After appending empty: '" + container1.getValue() + "'");
print("Still isEmpty: " + toString(container1.isEmpty()));

container1.append("content");
print("After appending content: '" + container1.getValue() + "'");
print("Now isEmpty: " + toString(container1.isEmpty()));

StringContainer container2 = new StringContainer("");
print("Container with empty string: isEmpty = " + toString(container2.isEmpty()));

// Test 11: Empty string method chaining
print("\nTesting method chaining with empty strings:");

class StringBuilder {
    string content;
    
    constructor() {
        content = "";
    }
    
    function append(string s): StringBuilder {
        content += s;
        return this;
    }
    
    function toString(): string {
        return content;
    }
}

StringBuilder builder = new StringBuilder();
string chainResult = builder.append("").append("hello").append("").append("world").append("").toString();
print("Chain with empty strings: '" + chainResult + "'");

// Test 12: Empty string with special characters
print("\nTesting empty string with special characters:");

string beforeSpecial = "";
string afterSpecial = beforeSpecial + "x";  // Add a regular character
print("Empty + 'x' length: " + toString(strlen(afterSpecial)));

string escapeEmpty = "";
string withEscape = escapeEmpty + "\\";
print("Empty + backslash: '" + withEscape + "'");

// Test 13: Empty string in nested functions
print("\nTesting empty strings in nested functions:");

function outer(string s): string {
    function inner(string innerS): string {
        if (innerS == "") {
            return "inner-empty";
        }
        return "inner-" + innerS;
    }
    
    if (s == "") {
        return inner("");
    }
    return inner(s);
}

print("Nested empty: " + outer(""));
print("Nested non-empty: " + outer("value"));

// Test 14: Empty string edge case with numbers
print("\nTesting empty string with number conversions:");

string emptyNum = "";
string numStr = emptyNum + toString(0);
print("Empty + toString(0): '" + numStr + "'");

string numStr2 = toString(42) + emptyNum;
print("toString(42) + empty: '" + numStr2 + "'");

// Test 15: Empty string in switch-like constructs
print("\nTesting empty string in switch-like logic:");

function processEmptyVariations(string s): string {
    if (s == "") {
        return "exactly empty";
    } else if (s == " ") {
        return "single space";
    } else if (strlen(s) == 0) {
        return "zero length";  // Should never reach here after first check
    } else {
        return "has content: " + s;
    }
}

print(processEmptyVariations(""));
print(processEmptyVariations(" "));
print(processEmptyVariations("text"));

// Test 16: Empty string memory and reference tests
print("\nTesting empty string references:");

string ref1 = "";
string ref2 = "";
string ref3 = ref1;

print("ref1 == ref2: " + toString(ref1 == ref2));
print("ref1 == ref3: " + toString(ref1 == ref3));
print("ref2 == ref3: " + toString(ref2 == ref3));

ref1 += "modified";
print("After modifying ref1:");
print("ref1: '" + ref1 + "'");
print("ref2: '" + ref2 + "'");
print("ref3: '" + ref3 + "'");

print("\nEmpty string edge cases test completed successfully");