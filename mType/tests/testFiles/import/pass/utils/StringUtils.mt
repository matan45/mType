// Level 1: String utilities
// Located in: utils/StringUtils.mt

public class StringBuilder {
    public function concat(string a, string b) : string {
        return a + b;
    }

    public function repeat(string s, int times) : string {
        string result = "";
        int i = 0;
        while (i < times) {
            result = result + s;
            i = i + 1;
        }
        return result;
    }
}

public function toUpperCase(string s) : string {
    // Simplified - just return as is for now
    return s;
}

public function trim(string s) : string {
    return s;
}

private function secretStringOp(string s) : string {
    return s + "_secret";
}

public int STRING_UTIL_VERSION = 2;
