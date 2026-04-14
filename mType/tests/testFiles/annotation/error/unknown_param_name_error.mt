// MYT-108 §7b case 5: passing a parameter name not declared on the
// annotation must produce a clear error naming the bad key.

annotation Timeout {
    int ms;
}

class Service {
    @Timeout(bogus = 1)
    public function ping(): void {
        print("never");
    }
}

new Service().ping();
