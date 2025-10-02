// Test: isClassOf with interfaces
interface Drawable {}
class Circle implements Drawable {}

Circle c = new Circle();
print(c isClassOf Circle);   // true
print(c isClassOf Drawable); // true

// Expected output:
// true
// true
