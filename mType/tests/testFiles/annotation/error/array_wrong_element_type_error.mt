// MYT-375: array usage rejects the wrong element type.

annotation Numbers {
    int[] values;
}

@Numbers(values = [1, "x"])
class Bad { }
