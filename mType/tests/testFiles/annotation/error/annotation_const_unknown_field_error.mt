// MYT-376: referencing a constant that does not exist on the class.

annotation Cfg {
    int x;
}

class Config {
    public static final int A = 1;
}

@Cfg(x = Config::MISSING)
class Target { }
