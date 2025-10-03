// Test: Protected method access violation from external context
class Processor {
    protected void process() {
        print("Processing...");
    }

    public void execute() {
        process();
    }
}

Processor p = new Processor();
p.process();  // ERROR: Cannot access protected method 'process'
