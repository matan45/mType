// Missing multiple methods from interface
interface Shape {
    function draw(): void;
    function getArea(): float;
    function getPerimeter(): float;
}

class IncompleteShape implements Shape {
    function draw(): void {
        print("Drawing shape");
    }
    // Missing getArea() and getPerimeter() methods
}

print("This should not print");