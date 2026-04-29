// MYT-217: Generic T? return type doesn't substitute when assigned to concrete T?.
//
// EXPECTED:
//   "found: 7"
//   "found: none"

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

ArrayList<Int> emptyNums = new ArrayList<Int>();
Int? miss = findFirst<Int>(emptyNums);
if (miss != null) {
    print("found: " + miss.getValue());
} else {
    print("found: none");
}
