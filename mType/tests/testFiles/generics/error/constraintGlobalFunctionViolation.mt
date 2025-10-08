// Test: Global function constraint violation
// Expected: Should fail - class doesn't implement required interface

interface Comparable<T> {
    function compareTo(T other): int;
}

class Product {
    private string name;

    constructor(string n) {
        this.name = n;
    }
    // Does NOT implement Comparable
}

function <T extends Comparable<T>> max(T a, T b): T {
    if (a.compareTo(b) > 0) {
        return a;
    }
    return b;
}

// ERROR: Product doesn't implement Comparable<Product>
Product p1 = new Product("Widget");
Product p2 = new Product("Gadget");
Product maxProduct = max<Product>(p1, p2);
