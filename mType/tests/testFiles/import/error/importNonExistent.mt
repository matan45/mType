// Test: Import from non-existent file
@Script
import { NonExistentClass } from "./this/file/does/not/exist.mt"

var obj = NonExistentClass();
