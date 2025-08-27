class Point {
    int x;
    int y;
    
    constructor(int px, int py) {
        x = px;
        y = py;
    }
    
    function distance(): float {
        // Distance from origin
        return x * x + y * y; // Simplified (should be sqrt)
    }
}

function createPoint(int x, int y): Point {
    return new Point(x, y);
}

function processPoint(Point p): int {
    return p.x + p.y;
}

function modifyPoint(Point p, int newX, int newY): void {
    p.x = newX;
    p.y = newY;
}

Point p1 = createPoint(3, 4);
print(p1.x); // 3
print(p1.y); // 4

int sum = processPoint(p1);
print(sum); // 7

modifyPoint(p1, 10, 20);
print(p1.x); // 10
print(p1.y); // 20

print(p1.distance()); // 500 (10*10 + 20*20)