// MYT-376: a function call is not a compile-time constant.

function compute(): int {
    return 5;
}

annotation Cfg {
    int x;
}

@Cfg(x = compute())
class Target { }
