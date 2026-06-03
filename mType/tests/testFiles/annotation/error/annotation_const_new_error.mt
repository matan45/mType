// MYT-376: a `new` construction is not a compile-time constant.

annotation Cfg {
    int x;
}

class Foo { }

@Cfg(x = new Foo())
class Target { }
