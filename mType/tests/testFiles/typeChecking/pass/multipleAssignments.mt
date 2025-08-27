class Shape {
    string type;
    constructor(string t) {
        type = t;
    }
}

class Color {
    string name;
    constructor(string n) {
        name = n;
    }
}

// Create multiple instances
Shape s1 = new Shape("Circle");
Shape s2 = new Shape("Square");
Shape s3 = new Shape("Triangle");

Color c1 = new Color("Red");
Color c2 = new Color("Blue");

// Valid assignments (same type)
s1 = s2;  // Circle becomes Square
s2 = s3;  // Square becomes Triangle  
c1 = c2;  // Red becomes Blue

print("Multiple assignments successful");