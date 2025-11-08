// Module with only private members (if mType supports private)
private class PrivateClass {
    private int secret;

    private constructor() {
        this.secret = 42;
    }

    private function getSecret(): int {
        return this.secret;
    }
}

// All members are private, nothing to export
