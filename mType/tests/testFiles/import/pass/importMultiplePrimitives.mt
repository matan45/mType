// Test: Import multiple primitives together

import { Int } from "../../lib/primitives/Int.mt";
import { String } from "../../lib/primitives/String.mt";
import { Bool } from "../../lib/primitives/Bool.mt";

Int num = 100;
String text = "Testing";
Bool flag = true;

print(num.toString());
print(text.toString());
print(flag.toString());
