// Test: Interface cannot have private methods (parse-time error)
interface Service {
    private void execute();  // ERROR: Interface methods must be public
}
