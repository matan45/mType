// MYT-376: a `Class::FIELD` reference must point at a STATIC FINAL field; a
// static non-final field is rejected.

annotation Cfg {
    int x;
}

class Config {
    public static int X = 5;
}

@Cfg(x = Config::X)
class Target { }
