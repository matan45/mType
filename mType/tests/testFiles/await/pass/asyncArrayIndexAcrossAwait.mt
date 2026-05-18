// Pins that an array reference and its index local both survive a
// real event-loop suspension. The slot arr[i] is read pre-await,
// arr[i+1] written post-await; both ops must hit the same array
// the local pointed at before suspension.

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Array Index Across Await ===");

function async main(): Promise<Int> {
    int[] arr = [10, 20, 30, 40];
    int i = 1;

    int before = arr[i];
    print("before: arr[" + i + "]=" + before);

    await delay(1);

    arr[i + 1] = before + 5;
    print("after: arr[" + (i + 1) + "]=" + arr[i + 1]);
    print("arr[0]=" + arr[0] + " arr[1]=" + arr[1] + " arr[2]=" + arr[2] + " arr[3]=" + arr[3]);
    print("OK");
    return new Int(arr[i + 1]);
}

main();
