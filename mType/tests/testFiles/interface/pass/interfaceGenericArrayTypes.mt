// Test interface methods with T[] or Array<T>
// @Script

interface ArrayProcessor<T> {
    func process(items: Array<T>): Array<T>;
    func filter(items: Array<T>, predicate: func(T): Bool): Array<T>;
    func first(items: Array<T>): T;
}

class StringArrayProcessor implements ArrayProcessor<String> {
    func process(items: Array<String>): Array<String> {
        var result = new Array<String>();
        var i = 0;
        while (i < items.size()) {
            var item = items.get(i);
            result.add(item + "!");
            i = i + 1;
        }
        return result;
    }

    func filter(items: Array<String>, predicate: func(String): Bool): Array<String> {
        var result = new Array<String>();
        var i = 0;
        while (i < items.size()) {
            var item = items.get(i);
            if (predicate(item)) {
                result.add(item);
            }
            i = i + 1;
        }
        return result;
    }

    func first(items: Array<String>): String {
        return items.get(0);
    }
}

var processor = new StringArrayProcessor();
var words = new Array<String>();
words.add("Hello");
words.add("World");
words.add("Test");

var processed = processor.process(words);
print(processed.get(0));  // Should print "Hello!"

var filtered = processor.filter(words, func(s: String): Bool {
    return s.length() > 4;
});
print(filtered.get(0));   // Should print "Hello" or "World"

var firstWord = processor.first(words);
print(firstWord);         // Should print "Hello"
