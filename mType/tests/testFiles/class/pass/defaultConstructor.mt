class Point {
    int x;
    int y;
    
    constructor() {
        x = 0;
        y = 0;
        print(999); // Constructor called
    }
}

Point p = new Point(); // Should print 999
print(p.x); // 0
print(p.y); // 0