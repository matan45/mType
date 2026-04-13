import * from "../../lib/primitives/Int.mt";
import * from "../../lib/collections/ArrayList.mt";

ArrayList<Int> numbers = new ArrayList<Int>();
numbers.add(new Int(1));
numbers.add(new Int(2));
numbers.add(new Int(3));
print(numbers.size());
print(numbers.get(0));
print(numbers.get(2));
