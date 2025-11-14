// Test interface methods with T[] or Array<T>
// @Script

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Bool.mt";


interface ArrayProcessor<T> {
    function process(ArrayList<T> items): ArrayList<T>;
    function filter(ArrayList<T> items, Function<T, Bool> predicate): ArrayList<T>;
    function first(ArrayList<T> items): T;
}

class StringArrayProcessor implements ArrayProcessor<String> {
    public function process(ArrayList<String> items): ArrayList<String> {
        ArrayList<String> result = new ArrayList<String>();
        for (int i = 0; i < items.size(); i++) {
            String item = items.get(i);
            result.add(new String(item.getValue() + "!"));
        }
        return result;
    }

    public function filter(ArrayList<String> items, Function<String, Bool> predicate): ArrayList<String> {
        ArrayList<String> result = new ArrayList<String>();
        for (int i = 0; i < items.size(); i++) {
            String item = items.get(i);
            if (predicate.apply(item).getValue()) {
                result.add(item);
            }
        }
        return result;
    }

    public function first(ArrayList<String> items): String {
        return items.get(0);
    }
}

StringArrayProcessor processor = new StringArrayProcessor();
ArrayList<String> words = new ArrayList<String>();
words.add(new String("Hello"));
words.add(new String("World"));
words.add(new String("Test"));

ArrayList<String> processed = processor.process(words);
print(processed.get(0).toString());  // Should print "Hello!"

ArrayList<String> filtered = processor.filter(words, s -> new Bool(s.length() > 4));
print(filtered.get(0).toString());   // Should print "Hello" or "World"

String firstWord = processor.first(words);
print(firstWord.toString());         // Should print "Hello"
