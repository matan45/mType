// Pins covariant return-type narrowing at the override declaration
// site (distinct from the existing asyncPromiseCovariance.mt which
// only upcasts at the call site). The override declares its return
// as Promise<Circle> where the base declared Promise<Shape>; the
// returned Circle is observed both through the base-typed
// reference and through the derived-typed reference.

print("=== Async Override Returns More Specific ===");

class Shape {
    public string name;
    public constructor(string n) {
        this.name = n;
    }
    public function async make(): Promise<Shape> {
        return new Shape("shape");
    }
}

class Circle extends Shape {
    public int radius;
    public constructor(string n, int r): super(n) {
        this.radius = r;
    }
    public function async make(): Promise<Circle> {
        return new Circle("circle", 5);
    }
}

function async main(): Promise<Shape> {
    Circle factory = new Circle("factory", 0);

    Circle direct = await factory.make();
    print("direct: name=" + direct.name + " radius=" + direct.radius);

    Shape upcast = factory;
    Shape viaBase = await upcast.make();
    print("viaBase: name=" + viaBase.name);
    print("OK");
    return viaBase;
}

main();
