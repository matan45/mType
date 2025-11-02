// Test: Import multiple primitives together
@Script
import { Int } from "../../../../../../lib/primitives/Int.mt"
import { String } from "../../../../../../lib/primitives/String.mt"
import { Bool } from "../../../../../../lib/primitives/Bool.mt"

var num: Int = 100;
var text: String = "Testing";
var flag: Bool = true;

print(num);
print(text);
print(flag);
