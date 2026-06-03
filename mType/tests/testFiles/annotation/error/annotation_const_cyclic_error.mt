// MYT-376: cyclic compile-time constant dependency is detected.

annotation Cfg {
    int x;
}

class Config {
    public static final int A = Config::B;
    public static final int B = Config::A;
}

@Cfg(x = Config::A)
class Target { }
