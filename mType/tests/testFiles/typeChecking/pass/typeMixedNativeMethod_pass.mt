// Test native method type checking

import * from "../../lib/primitives/String.mt";


String result = "hello world";
print("toUpperCase: " + result.toUpperCase());
print("toLowerCase: " + result.toLowerCase());
print("Native method type checking passed");
