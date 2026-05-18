// Test: Mixed wildcard and selective import (invalid syntax)
import *, { SomeClass } from "./modules/SomeModule.mt"

var obj = SomeClass();
