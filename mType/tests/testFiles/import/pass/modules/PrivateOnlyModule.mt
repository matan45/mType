// Module with only private members (if mType supports private)
class PrivateClass {
    private var secret: Int;

    private constructor() {
        this.secret = 42;
    }

    private fun getSecret(): Int {
        return this.secret;
    }
}

// All members are private, nothing to export
