// Test: Interface cannot have protected methods (parse-time error)
interface Handler {
    protected void handle();  // ERROR: Interface methods must be public
}
