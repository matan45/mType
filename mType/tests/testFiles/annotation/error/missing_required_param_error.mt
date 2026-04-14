// MYT-108 §7b case 4: required (non-defaulted) parameter must be supplied.

annotation Timeout {
    int ms;
}

class Service {
    @Timeout
    public function ping(): void {
        print("never");
    }
}

new Service().ping();
