// MYT-382: break inside while and foreach loops nested in switch cases must
// exit the loop only. Covers the non-for loopStart-capture timing (while) and
// the foreach break path.
int sel = 0;
int i = 0;
switch (sel) {
    case 0:
        while (i < 5) {
            if (i == 3) {
                break;   // exits the while loop only
            }
            print(i);
            i = i + 1;
        }
        print("after while");
        break;
}

int[] nums = [10, 20, 30, 40];
switch (sel) {
    case 0:
        for (int n : nums) {
            if (n == 30) {
                break;   // exits the foreach loop only
            }
            print(n);
        }
        print("after foreach");
        break;
}
print("done");
