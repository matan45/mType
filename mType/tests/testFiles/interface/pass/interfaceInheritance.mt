// Interface inheritance test
interface Shape {
    function getArea(): float;
}

interface Drawable {
    function draw(): void;
}

interface DrawableShape extends Shape, Drawable {
    function getPerimeter(): float;
}

print("Interface inheritance successful");