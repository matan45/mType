// Test: Cast with null-coalescing patterns
class Product {
    public string name;
    public float price;

    constructor(string n, float p) {
        this.name = n;
        this.price = p;
    }
}

class DiscountedProduct extends Product {
    float discount;

    constructor(string n, float p, float d) : super(n, p) {
        this.discount = d;
    }

    public function getFinalPrice(): float {
        return this.price * (1.0 - this.discount);
    }
}

// Null coalescing with cast
Product p1 = null;
Product p2 = new Product("Item", 100.0);
Product result1 = p1 != null ? (Product)p1 : (Product)p2;
print(result1.name);

// Chained null coalescing with casts
Product p3 = null;
Product p4 = null;
Product p5 = new DiscountedProduct("Sale Item", 50.0, 0.2);

Product result2 = p3 != null ? (Product)p3 : (p4 != null ? (Product)p4 : (Product)p5);
print(result2.name);

// Cast to derived type with coalescing
DiscountedProduct dp1 = null;
DiscountedProduct dp2 = new DiscountedProduct("Special", 200.0, 0.3);

DiscountedProduct result3 = dp1 != null ? (DiscountedProduct)dp1 : (DiscountedProduct)dp2;
print(result3.getFinalPrice());

// Nullable cast chain with fallback
Product nullable1 = new DiscountedProduct("Promo", 100.0, 0.5);
DiscountedProduct discounted = nullable1 != null ? (DiscountedProduct)(Product)nullable1 : null;
float finalPrice = discounted != null ? discounted.getFinalPrice() : 0.0;
print(finalPrice);

// Multiple nullable casts with coalescing
Product base1 = null;
Product base2 = new Product("Regular", 75.0);
string productName = base1 != null ? ((Product)base1).name : ((Product)base2).name;
print(productName);

// Expected output:
// Item
// Sale Item
// 140.0
// 50.0
// Regular
