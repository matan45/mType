import * from "../../lib/primitives/Int.mt";
import * from "../../lib/collections/ArrayList.mt";

ArrayList<Int> numbers = new ArrayList<Int>();
numbers.add(new Int(1));
numbers.add(new Int(2));
numbers.add(new Int(3));
numbers.add(new Int(4));
numbers.add(new Int(5));

Int sum = numbers.stream()
    .filter(x -> x > 2)
    .map(x -> x * 10)
    .reduceWithIdentity(0, (a, b) -> a + b);

print(sum);
