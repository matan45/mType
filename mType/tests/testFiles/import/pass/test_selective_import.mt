// Test selective import syntax
import { Calculator, divide, subtract, Comparable } from "selective_import_utils.mt"

function main() : void {
    Calculator calc = new Calculator();
    int sum = calc.add(10, 20);
    int product = calc.multiply(5, 6);

    float quotient = divide(10.0, 2.0);
    int difference = subtract(50, 25);

    print("Sum: " + string(sum));
    print("Product: " + string(product));
    print("Quotient: " + string(quotient));
    print("Difference: " + string(difference));
}

main();
