// Test: Protected method access from non-subclass
class ServiceA {
    protected function doWork(): void {
        print("Working...");
    }
}

class ServiceB {
    public function callWork(ServiceA service): void {
        service.doWork();  // ERROR: Cannot access protected method (not a subclass)
    }
}

ServiceA sA = new ServiceA();
ServiceB sB = new ServiceB();
sB.callWork(sA);
