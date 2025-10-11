// Test that importing non-existent symbols fails
// This should fail because NonExistentClass doesn't exist
import { Calculator, NonExistentClass } from "selective_import_utils.mt"

function main() : void {
    Calculator calc = new Calculator();
    print("Calculator created");
}

main();
