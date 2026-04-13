class Secret {
    private int code;

    public constructor(int code) {
        this.code = code;
    }
}

Secret s = new Secret(42);
print(s.code);
