// Class + Generics Test 4: Generic builder pattern
@Script

class Product<T> {
    field name: String;
    field value: T;
    field quantity: Int;
    field active: Bool;

    constructor() {
        this.active = false;
        this.quantity = 0;
    }

    fun setName(n: String): Void {
        this.name = n;
    }

    fun setValue(v: T): Void {
        this.value = v;
    }

    fun setQuantity(q: Int): Void {
        this.quantity = q;
    }

    fun setActive(a: Bool): Void {
        this.active = a;
    }

    fun display(): Void {
        print(this.name);
        print(this.value);
        print(this.quantity);
        print(this.active);
    }
}

class ProductBuilder<T> {
    field product: Product<T>;

    constructor() {
        this.product = Product<T>();
    }

    fun withName(name: String): ProductBuilder<T> {
        this.product.setName(name);
        return this;
    }

    fun withValue(value: T): ProductBuilder<T> {
        this.product.setValue(value);
        return this;
    }

    fun withQuantity(qty: Int): ProductBuilder<T> {
        this.product.setQuantity(qty);
        return this;
    }

    fun withActive(active: Bool): ProductBuilder<T> {
        this.product.setActive(active);
        return this;
    }

    fun build(): Product<T> {
        return this.product;
    }
}

print("Building Product<Int>:");
let intBuilder: ProductBuilder<Int> = ProductBuilder<Int>();
let intProduct: Product<Int> = intBuilder
    .withName("Widget")
    .withValue(100)
    .withQuantity(50)
    .withActive(true)
    .build();

intProduct.display();

print("Building Product<String>:");
let strBuilder: ProductBuilder<String> = ProductBuilder<String>();
let strProduct: Product<String> = strBuilder
    .withName("Service")
    .withValue("Premium")
    .withQuantity(1)
    .withActive(false)
    .build();

strProduct.display();

print("Building Product<Float>:");
let floatBuilder: ProductBuilder<Float> = ProductBuilder<Float>();
let floatProduct: Product<Float> = floatBuilder
    .withName("Price")
    .withValue(99.99)
    .withQuantity(0)
    .withActive(true)
    .build();

floatProduct.display();
