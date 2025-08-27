class Product {
    string name;
    float price;
    
    constructor(string n, float p) {
        name = n;
        price = p;
    }
    
    function getPrice(): float {
        return price;
    }
}

namespace factory {
    function createProduct(string name, float price): Product {
        return new Product(name, price);
    }
}

Product prod = factory::createProduct("Widget", 29.99);
float prodPrice = prod.getPrice();

print("Namespace function returning object test passed");