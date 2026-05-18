// Test: Invalid special characters in import path
import { SomeClass } from "./modules/<invalid>|path?.mt"

var obj = SomeClass();
