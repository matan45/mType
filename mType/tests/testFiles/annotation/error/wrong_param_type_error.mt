// MYT-108 §7b case 2: passing a string where an int is declared must error.

annotation Timeout {
    int ms;
}

class Service {
    @Timeout(ms = "fast")
    public function ping(): void {
        print("never");
    }
}

new Service().ping();
