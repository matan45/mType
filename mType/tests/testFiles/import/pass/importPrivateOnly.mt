// Test: Import from file with only private symbols
// This should pass but PrivateClass should not be accessible
@Script
import * from "./modules/PrivateOnlyModule.mt"

print("Imported from private-only module");
// Attempting to use PrivateClass should fail if we tried
