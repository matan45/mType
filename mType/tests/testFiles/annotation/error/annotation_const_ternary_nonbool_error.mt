// MYT-376: a ternary whose condition does not fold to a boolean constant is
// rejected — the condition here folds to int 5, not a bool.

annotation Cfg {
    int x;
}

@Cfg(x = 5 ? 1 : 2)
class Target { }
