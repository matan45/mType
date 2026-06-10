// MYT-382: break inside a loop under try/finally, inside a switch case. The
// inner-loop break must exit only the loop after running the finally, then
// reach the rest of the case. Exercises the now-reachable break-trampoline
// (exceptionManager.registerBreakJump) for loop-in-switch.
int sel = 0;
switch (sel) {
    case 0:
        for (int i = 0; i < 5; i = i + 1) {
            try {
                if (i == 2) {
                    break;   // exits the for loop after finally runs
                }
                print("iter " + i);
            } finally {
                print("finally " + i);
            }
        }
        print("after loop");
        break;
}
print("done");
