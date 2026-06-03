// MYT-376: a constant expression that cannot be folded (division by zero) is
// rejected rather than silently producing a value.

annotation Cfg {
    int x;
}

@Cfg(x = 1 / 0)
class Target { }
