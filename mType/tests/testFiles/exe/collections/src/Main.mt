// Test: Collections in standalone exe
import * from "../../../../lib/collections/ArrayList.mt";
import * from "../../../../lib/collections/HashMap.mt";
import * from "../../../../lib/collections/HashSet.mt";
import * from "../../../../lib/collections/Stack.mt";
import * from "../../../../lib/collections/ArrayQueue.mt";
import * from "../../../../lib/primitives/Int.mt";
import * from "../../../../lib/primitives/String.mt";

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

        // HashMap
        HashMap<String, Int> map = new HashMap<String, Int>();
        map.put(new String("a"), new Int(1));
        map.put(new String("b"), new Int(2));
        map.put(new String("c"), new Int(3));
        print("HashMap size: " + map.size());
        print("HashMap[b]: " + map.get(new String("b")));

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
