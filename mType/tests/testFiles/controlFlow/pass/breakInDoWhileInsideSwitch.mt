// MYT-382: break inside a do-while nested in a switch case must exit the
// do-while only. Exercises do-while's loopStart (captured before enterLoop)
// against the switch entry offset.
int sel = 0;
int i = 0;
switch (sel) {
    case 0:
        do {
            i = i + 1;
            if (i == 3) {
                break;   // exits the do-while only
            }
            print(i);
        } while (i < 10);
        print("after do-while");
        break;
}
print("done");
