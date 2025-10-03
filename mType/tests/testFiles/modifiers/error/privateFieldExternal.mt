// Test: Private field access violation from external context
class Secret {
    private int code;

    public Secret(int c) {
        code = c;
    }
}

Secret s = new Secret(1234);
print(s.code);  // ERROR: Cannot access private field 'code'
