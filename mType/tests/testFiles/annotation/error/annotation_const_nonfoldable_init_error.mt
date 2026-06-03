// MYT-376: a static final field whose initializer is not constant-foldable
// cannot be used as an annotation argument.

function compute(): int {
    return 5;
}

annotation Cfg {
    int x;
}

class Config {
    public static final int X = compute();
}

@Cfg(x = Config::X)
class Target { }
