// MYT-382: break inside a for loop nested in a switch case must exit the
// loop only, not the enclosing switch. After the loop breaks, control must
// continue with the rest of the case body (here "after loop in case 0")
// before the case's own break exits the switch.
int x = 0;
switch (x) {
    case 0:
        for (int i = 0; i < 5; i = i + 1) {
            if (i == 2) {
                break;   // exits the for loop, NOT the switch
            }
            print(i);
        }
        print("after loop in case 0");
        break;           // exits the switch
    case 1:
        print("case 1");
        break;
}
print("after switch");
