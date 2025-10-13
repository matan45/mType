// Error: Duplicate interface name declaration

interface Drawable {
    function draw(): void;
}

// ERROR: Duplicate interface with same name
interface Drawable {
    function render(): void;
}
