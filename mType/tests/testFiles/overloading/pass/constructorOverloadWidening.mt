// Test: Constructor overloading with widening - exact type should win
class Shape {
    string desc;

    constructor(int sides) {
        this.desc = "Shape with " + sides + " sides";
    }

    constructor(float area) {
        this.desc = "Shape with area " + area;
    }

    constructor(string name) {
        this.desc = "Shape named " + name;
    }

    public function getDesc(): string {
        return this.desc;
    }
}

function main(): void {
    print("=== Constructor Overload Widening ===");

    Shape s1 = new Shape(4);           // exact int match
    Shape s2 = new Shape(3.14);        // exact float match
    Shape s3 = new Shape("triangle");  // exact string match

    print(s1.getDesc());
    print(s2.getDesc());
    print(s3.getDesc());
}
main();
