// Test: Class members default to private
class SecretBox {
    int secret;  // No modifier = private by default

    public function SecretBox(int s): void {
        secret = s;  // Private field accessible in same class
    }

    public function reveal(): int {
        return secret;  // Private field accessible in same class
    }
}

SecretBox box = new SecretBox(42);
print(box.reveal());  // Expected: 42
// Cannot access box.secret directly (would be error)
