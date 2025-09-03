

    class Point {
        float x;
        float y;
        
        constructor(float px, float py) {
            x = px;
            y = py;
        }
        
        function distanceFromOrigin(): float {
            return sqrt(x * x + y * y);
        }
        
        function getCoordinates(): string {
            return "(" + toString(x) + ", " + toString(y) + ")";
        }
        
        static function createOrigin(): Point {
            return new Point(0.0, 0.0);
        }
    }
    
    class Shape {
        static int shapeCount = 0;
        static final string category = "2D Shape";
        
        static function getShapeCount(): int {
            return shapeCount;
        }
        
        static function incrementShapeCount(): void {
            shapeCount = shapeCount + 1;
        }
        
        static function getCategory(): string {
            return category;
        }
    }



    class Vector {
        float x;
        float y;
        float z;
        
        constructor(float vx, float vy, float vz) {
            x = vx;
            y = vy;
            z = vz;
        }
        
        function magnitude(): float {
            return sqrt(x * x + y * y + z * z);
        }
		
		function magnitude(float update): void {
            
        }
        
        static function zero(): Vector {
            return new Vector(0.0, 0.0, 0.0);
        }
        
        static function simpleTest(): int {
            return 42;
        }
    }


// Class with different field types
class Student {
    string name;
    int age;
    float gpa;
    bool isActive;
    
    constructor(string n, int a) {
        name = n;
        age = a;
        gpa = 0.01;
        isActive = true;
    }
    
    constructor(string n, int a, float g) {
        name = n;
        age = a;
        gpa = g;
        isActive = true;
    }
    
    function displayInfo(): string {
        return this.name + " (Age: " + toString(this.age) + ", GPA: " + toString(this.gpa) + ")";
    }
}

// Class with static fields
class University {
    static string universityName = "Tech University";
    static int totalStudents = 0;
    static final int maxCapacity = 10000;
    
    static function incrementStudents(): void {
        totalStudents = totalStudents + 1;
    }
    
    static function getInfo(): string {
        return universityName + " - Students: " + toString(totalStudents) + "/" + toString(maxCapacity);
    }
}

// Class with static methods and fields
class MathUtils {
    static final float PI = 3.14159;
    static int operationCount = 0;
    
    static function square(int x): int {
        operationCount = operationCount + 1;
        return x * x;
    }
    
    static function circleArea(float radius): float {
        operationCount = operationCount + 1;
        return PI * radius * radius;
    }
    
    static function getOperationCount(): int {
        return operationCount;
    }
    
    static function resetCount(): void {
        operationCount = 0;
    }
}

// Another class with static utilities
class StringUtils {
    static function reverse(string str): string {
        // Simple reverse implementation for demo
        return str + " (reversed)";  // Simplified for demo
    }
    
    static function repeat(string str, int times): string {
        string result = "";
        for (int i = 0; i < times; i = i + 1) {
            result = result + str;
        }
        return result;
    }
}


Point pt = createPoint(4,5);//native c++ create Pint object