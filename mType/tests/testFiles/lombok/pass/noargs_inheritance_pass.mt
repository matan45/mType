// A child @NoArgsConstructor must chain to the parent: buildNoArgsConstructor
// emits super() when the class has a parent. Reading the inherited field's
// default through the parent's @Getter proves the child's no-arg ctor compiled
// and ran the super() chain.
@NoArgsConstructor
@Getter
class Base {
    protected string label = "base";
}

@NoArgsConstructor
class Derived extends Base {
    private int extra = 7;
}

Derived d = new Derived();
print(d.getLabel());

// Expected output:
// base
