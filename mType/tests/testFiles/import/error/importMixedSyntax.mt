// Test: Mixed wildcard and selective import (invalid syntax)
@Script
import *, { SomeClass } from "./modules/SomeModule.mt"

var obj = SomeClass();
