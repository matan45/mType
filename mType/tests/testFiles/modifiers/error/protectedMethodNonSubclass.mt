// Test: Protected method access from non-subclass
class ServiceA {
    protected void doWork() {
        print("Working...");
    }
}

class ServiceB {
    public void callWork(ServiceA service) {
        service.doWork();  // ERROR: Cannot access protected method (not a subclass)
    }
}

ServiceA sA = new ServiceA();
ServiceB sB = new ServiceB();
sB.callWork(sA);
