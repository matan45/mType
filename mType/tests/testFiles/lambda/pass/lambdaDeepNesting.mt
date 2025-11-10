// 10+ nested lambda levels test
interface Function {
    function apply(int x) : int;
}

print("=== Deep Nesting Test ===");

// 10 nested lambdas
Function level1 = x -> {
    Function level2 = y -> {
        Function level3 = z -> {
            Function level4 = a -> {
                Function level5 = b -> {
                    Function level6 = c -> {
                        Function level7 = d -> {
                            Function level8 = e -> {
                                Function level9 = f -> {
                                    Function level10 = g -> {
                                        return x + y + z + a + b + c + d + e + f + g;
                                    };
                                    return level10.apply(f + 1);
                                };
                                return level9.apply(e + 1);
                            };
                            return level8.apply(d + 1);
                        };
                        return level7.apply(c + 1);
                    };
                    return level6.apply(b + 1);
                };
                return level5.apply(a + 1);
            };
            return level4.apply(z + 1);
        };
        return level3.apply(y + 1);
    };
    return level2.apply(x + 1);
};

print("Deep nested result: " + level1.apply(1));

// 12 nested lambdas with computation
int captured1 = 1;
int captured2 = 2;
int captured3 = 3;

Function deep = a -> {
    int local1 = a + captured1;
    Function f2 = b -> {
        int local2 = b + captured2;
        Function f3 = c -> {
            int local3 = c + captured3;
            Function f4 = d -> {
                Function f5 = e -> {
                    Function f6 = f -> {
                        Function f7 = g -> {
                            Function f8 = h -> {
                                Function f9 = i -> {
                                    Function f10 = j -> {
                                        Function f11 = k -> {
                                            Function f12 = l -> {
                                                return local1 + local2 + local3 + d + e + f + g + h + i + j + k + l;
                                            };
                                            return f12.apply(k * 2);
                                        };
                                        return f11.apply(j * 2);
                                    };
                                    return f10.apply(i * 2);
                                };
                                return f9.apply(h * 2);
                            };
                            return f8.apply(g * 2);
                        };
                        return f7.apply(f * 2);
                    };
                    return f6.apply(e * 2);
                };
                return f5.apply(d * 2);
            };
            return f4.apply(1);
        };
        return f3.apply(1);
    };
    return f2.apply(1);
};

print("Deep nested with captures: " + deep.apply(1));

// Nested lambdas with arrays
Function arrayNesting = x -> {
    int[] arr = [x];
    Function f1 = y -> {
        arr[0] = arr[0] + y;
        Function f2 = z -> {
            arr[0] = arr[0] + z;
            Function f3 = w -> {
                arr[0] = arr[0] + w;
                return arr[0];
            };
            return f3.apply(z);
        };
        return f2.apply(y);
    };
    return f1.apply(x);
};

print("Array nesting result: " + arrayNesting.apply(5));

print("Deep nesting complete");
