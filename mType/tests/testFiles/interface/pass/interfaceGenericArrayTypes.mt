// Test interface methods with T[] or Array<T>
// @Script

import * from "../../lib/collections/List.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Bool.mt";

interface Function<T,R>{
	function predicate(T value): R;
}

interface ArrayProcessor<T> {
    function process(List<T> items): List<T>;
    function filter(List<T> items, Function<T, Bool> predicate): List<T>;
    function first(List<T> items): T;
}

class StringArrayProcessor implements ArrayProcessor<String> {
    public function process(List<String> items): List<String> {
        List<String> result = new List<String>();
        for (int i = 0; i < items.size(); i++) {
            String item = items.get(i);
            result.add(new String(item.value + "!"));
        }
        return result;
    }

    public function filter(List<String> items, Function<String, Bool> predicate): List<String> {
        List<String> result = new List<String>();
        for (int i = 0; i < items.size(); i++) {
            String item = items.get(i);
            if (predicate.predicate(item)) {
                result.add(item);
            }
        }
        return result;
    }

    public function first(List<String> items): String {
        return items.get(0);
    }
}

StringArrayProcessor processor = new StringArrayProcessor();
List<String> words = new List<String>();
words.add(new String("Hello"));
words.add(new String("World"));
words.add(new String("Test"));

List<String> processed = processor.process(words);
print(processed.get(0).toString());  // Should print "Hello!"

List<String> filtered = processor.filter(words, s -> new Bool(s.length() > 4));
print(filtered.get(0).toString());   // Should print "Hello" or "World"

String firstWord = processor.first(words);
print(firstWord.toString());         // Should print "Hello"
