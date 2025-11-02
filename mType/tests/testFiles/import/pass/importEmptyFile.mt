// Test: Import from an empty file (should pass but nothing is imported)
@Script
import * from "./modules/EmptyModule.mt"

print("Imported from empty file successfully");
