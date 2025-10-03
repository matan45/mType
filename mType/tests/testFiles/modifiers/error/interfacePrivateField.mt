// Test: Interface cannot have private fields (parse-time error)
interface Config {
    private string value;  // ERROR: Interface members must be public
}
