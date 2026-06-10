// MYT-383: continue in a do-while must jump to the CONDITION (re-evaluating
// it), not back to the body start. The condition here calls check(), which
// prints, so a skipped condition is directly observable: the i==2 iteration
// must still evaluate check(2) even though its body is skipped by continue.
function check(int v): bool {
    print("check " + v);
    return v < 3;
}

int i = 0;
do {
    i = i + 1;
    if (i == 2) {
        continue;
    }
    print("body " + i);
} while (check(i));
print("done");
