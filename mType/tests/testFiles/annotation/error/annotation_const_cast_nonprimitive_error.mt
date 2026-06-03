// MYT-376: a constant cast must target a primitive type (int, float, bool,
// String). Casting to a class type is not foldable and is rejected.

annotation Cfg {
    int x;
}

class Foo { }

@Cfg(x = (Foo)5)
class Target { }
