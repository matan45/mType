// Test: open-form and closed-form Class handles for the same base class are
// distinct (matching C#'s typeof(List<>) != typeof(List<int>)).

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Box.mt";

Class boxOpen = Class::forName("Box");
Class boxInt = Class::forName("Box<Int>");

int openHandle = boxOpen.getNativeHandle();
int closedHandle = boxInt.getNativeHandle();

print("Open handle positive: " + (openHandle > 0));
print("Closed handle positive: " + (closedHandle > 0));
print("Handles differ: " + (openHandle != closedHandle));

print("Test passed");
