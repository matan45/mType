// Interface instantiation error test (interfaces cannot be instantiated)
interface Drawable {
    function draw(): void;
}

Drawable obj = new Drawable(); // This should cause an error

print("This should not print");