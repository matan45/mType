// Test: Invalid special characters in import path
@Script
import { SomeClass } from "./modules/<invalid>|path?.mt"

var obj = SomeClass();
