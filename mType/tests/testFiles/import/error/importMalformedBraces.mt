// Test: Malformed braces - missing comma between imports
@Script
import { ClassX ClassY } from "./modules/SomeModule.mt"

var obj = ClassX();
