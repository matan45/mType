// Test: Protected method access violation from external context
class Processor {
    protected function process(): void {
        print("Processing...");
    }

    public function execute(): void {
        process();
    }
}

Processor p = new Processor();
p.process();  // ERROR: Cannot access protected method 'process'
