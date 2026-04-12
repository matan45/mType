// Test: Collections in standalone exe
import * from "collections/ArrayList.mt";
import * from "collections/HashSet.mt";
import * from "collections/Stack.mt";
import * from "collections/ArrayQueue.mt";
import * from "primitives/Int.mt";
import * from "primitives/String.mt";

@EntryPoint
class App {
    public static function main(string[] args): void {
        // ArrayList
        ArrayList<Int> list = new ArrayList<Int>();
        list.add(new Int(10));
        list.add(new Int(20));
        list.add(new Int(30));
        print("ArrayList size: " + list.size());
        print("ArrayList[1]: " + list.get(1));

        // ArrayList iteration
        print("ArrayList contents:");
        for (Int item : list) {
            print("  " + item);
        }

        // HashSet
        HashSet<String> set = new HashSet<String>();
        set.add(new String("x"));
        set.add(new String("y"));
        set.add(new String("x"));
        print("HashSet size: " + set.size());
        print("HashSet contains x: " + set.contains(new String("x")));

        // Stack
        Stack<Int> stack = new Stack<Int>();
        stack.push(new Int(100));
        stack.push(new Int(200));
        stack.push(new Int(300));
        print("Stack peek: " + stack.peek());
        Int popped = stack.pop();
        print("Stack pop: " + popped);
        print("Stack size after pop: " + stack.size());

        // Queue
        ArrayQueue<Int> queue = new ArrayQueue<Int>();
        queue.enqueue(new Int(1));
        queue.enqueue(new Int(2));
        queue.enqueue(new Int(3));
        print("Queue peek: " + queue.peek());
        Int dequeued = queue.dequeue();
        print("Queue dequeue: " + dequeued);
        print("Queue size after dequeue: " + queue.size());

        print("Collections test passed");
    }
}
