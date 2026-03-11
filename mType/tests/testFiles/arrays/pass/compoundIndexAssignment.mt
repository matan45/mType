// Test compound assignment operators on array indices


function main(): void {
    // Test += on int array
    int[] nums = new int[5];
    nums[0] = 10;
    nums[0] += 5;
    print("nums[0] after +=: " + nums[0]);

    // Test -= on int array
    nums[1] = 20;
    nums[1] -= 7;
    print("nums[1] after -=: " + nums[1]);

    // Test *= on int array
    nums[2] = 5;
    nums[2] *= 3;
    print("nums[2] after *=: " + nums[2]);

    // Test /= on int array
    nums[3] = 20;
    nums[3] /= 4;
    print("nums[3] after /=: " + nums[3]);

    // Test %= on int array
    nums[4] = 17;
    nums[4] %= 5;
    print("nums[4] after %=: " + nums[4]);

    // Test += on float array
    float[] floats = new float[3];
    floats[0] = 2.5;
    floats[0] += 1.5;
    print("floats[0] after +=: " + floats[0]);

    floats[1] = 10.0;
    floats[1] -= 3.5;
    print("floats[1] after -=: " + floats[1]);

    floats[2] = 2.5;
    floats[2] *= 4.0;
    print("floats[2] after *=: " + floats[2]);

    // Test += on string array (concatenation)
    string[] strs = new string[2];
    strs[0] = "Hello";
    strs[0] += " World";
    print("strs[0] after +=: " + strs[0]);

    strs[1] = "foo";
    strs[1] += "bar";
    print("strs[1] after +=: " + strs[1]);

    // Test compound assignment with variable index
    int[] arr = new int[3];
    arr[0] = 100;
    arr[1] = 200;
    arr[2] = 300;
    int idx = 1;
    arr[idx] += 50;
    print("arr[idx] after +=: " + arr[idx]);

    // Test compound assignment in a loop
    int[] counters = new int[4];
    for (int i = 0; i < 4; i++) {
        counters[i] = 0;
    }
    for (int i = 0; i < 4; i++) {
        counters[i] += (i + 1) * 10;
    }
    print("counters[0]: " + counters[0]);
    print("counters[1]: " + counters[1]);
    print("counters[2]: " + counters[2]);
    print("counters[3]: " + counters[3]);

    // Test multiple compound ops on same index
    int[] vals = new int[1];
    vals[0] = 100;
    vals[0] += 10;
    vals[0] -= 5;
    vals[0] *= 2;
    vals[0] /= 3;
    print("vals[0] after chain: " + vals[0]);
}

main();
