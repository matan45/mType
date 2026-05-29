// @Getter over an array-typed field returns the array type intact, so element
// access and .length work through the synthesized accessor. @AllArgsConstructor
// takes the array as its parameter. (Array .toString() isn't golden-stable, so
// this reads elements instead.)
@Getter
@AllArgsConstructor
class Bag {
    private int[] items;
}

int[] xs = new int[3];
xs[0] = 10;
xs[1] = 20;
xs[2] = 30;

Bag b = new Bag(xs);
print(b.getItems()[0]);
print(b.getItems().length);

// Expected output:
// 10
// 3
