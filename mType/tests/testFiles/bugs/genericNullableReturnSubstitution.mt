// MYT-217: Generic T? return type doesn't substitute when assigned to concrete T?.
//
// EXPECTED:
//   "found: 7"  (findFirst<Int> substitutes T?=Int?, assignment to Int? works)
//
// ACTUAL (broken):
//   Type error: cannot assign T? to Int
//   (Even with explicit <Int>, the return type is reported as the unsubstituted
//   T?, and the LHS Int? is reported just as "Int" in the error message.)

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

function <T> findFirst(ArrayList<T> list): T? {
    if (list.size() == 0) {
        return null;
    }
    return (T)list.get(0);
}

ArrayList<Int> nums = new ArrayList<Int>();
nums.add(new Int(7));

Int? hit = findFirst<Int>(nums);
if (hit != null) {
    print("found: " + hit.getValue());
} else {
    print("found: none");
}
