// Class + Generics Test 4: Generic builder pattern
@Script

class Product<T> {
    String name;
    T value;
    Int quantity;
    Bool active;

    constructor() {
        this.active = false;
        this.quantity = 0;
    }

    public function setName(String n): void {
        this.name = n;
    }

    public function setValue(T v): void {
        this.value = v;
    }

    public function setQuantity(Int q): void {
        this.quantity = q;
    }

    public function setActive(Bool a): void {
        this.active = a;
    }

    public function display(): void {
        print(this.name);
        print(this.value);
        print(this.quantity);
        print(this.active);
    }
}

class ProductBuilder<T> {
    Product<T> product;

    constructor() {
        this.product = Product<T>();
    }

    public function withName(String name): ProductBuilder<T> {
        this.product.setName(name);
        return this;
    }

    public function withValue(T value): ProductBuilder<T> {
        this.product.setValue(value);
        return this;
    }

    public function withQuantity(Int qty): ProductBuilder<T> {
        this.product.setQuantity(qty);
        return this;
    }

    public function withActive(Bool active): ProductBuilder<T> {
        this.product.setActive(active);
        return this;
    }

    public function build(): Product<T> {
        return this.product;
    }
}

print("Building Product<Int>:");
ProductBuilder<Int> intBuilder = ProductBuilder<Int>();
Product<Int> intProduct = intBuilder
    .withName("Widget")
    .withValue(100)
    .withQuantity(50)
    .withActive(true)
    .build();

intProduct.display();

print("Building Product<String>:");
ProductBuilder<String> strBuilder = ProductBuilder<String>();
Product<String> strProduct = strBuilder
    .withName("Service")
    .withValue("Premium")
    .withQuantity(1)
    .withActive(false)
    .build();

strProduct.display();

print("Building Product<Float>:");
ProductBuilder<Float> floatBuilder = ProductBuilder<Float>();
Product<Float> floatProduct = floatBuilder
    .withName("Price")
    .withValue(99.99)
    .withQuantity(0)
    .withActive(true)
    .build();

floatProduct.display();
