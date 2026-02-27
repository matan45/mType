// Test full roundtrip: create -> serialize -> deserialize -> verify

import * from "../../lib/json/Json.mt";

class Point {
    public int x;
    public int y;

    public constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }
}

class Shape {
    public string name;
    public Point origin;
    public int sides;

    public constructor(string name, Point origin, int sides) {
        this.name = name;
        this.origin = origin;
        this.sides = sides;
    }
}

// Create
Point p = new Point(10, 20);
Shape s = new Shape("Triangle", p, 3);

// Serialize
string json = Json::serialize(s);
print(json);

// Deserialize
Shape restored = Json::deserializeAs(json, "Shape");

// Verify
print(restored.name);
print(restored.sides);
print(restored.origin.x);
print(restored.origin.y);

print("Test passed");
