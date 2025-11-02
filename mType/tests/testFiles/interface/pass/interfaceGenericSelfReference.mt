// Test generic self-reference pattern (Comparable)
// @Script

interface Comparable<T extends Comparable<T>> {
    func compareTo(other: T): Int;
    func isGreaterThan(other: T): Bool;
}

class Score implements Comparable<Score> {
    var points: Int;

    func init(points: Int) {
        this.points = points;
    }

    func compareTo(other: Score): Int {
        if (this.points < other.points) {
            return -1;
        }
        if (this.points > other.points) {
            return 1;
        }
        return 0;
    }

    func isGreaterThan(other: Score): Bool {
        return this.compareTo(other) > 0;
    }
}

class Ranking<T extends Comparable<T>> {
    var items: Array<T>;

    func init() {
        this.items = new Array<T>();
    }

    func add(item: T): void {
        this.items.add(item);
    }

    func findHighest(): T {
        if (this.items.size() == 0) {
            return null;
        }

        var highest = this.items.get(0);
        var i = 1;
        while (i < this.items.size()) {
            var current = this.items.get(i);
            if (current.isGreaterThan(highest)) {
                highest = current;
            }
            i = i + 1;
        }
        return highest;
    }
}

var ranking = new Ranking<Score>();
ranking.add(new Score(100));
ranking.add(new Score(250));
ranking.add(new Score(175));

var highest = ranking.findHighest();
if (highest != null) {
    print("Highest score: " + highest.points.toString());
}

var s1 = new Score(100);
var s2 = new Score(200);

if (s2.isGreaterThan(s1)) {
    print("s2 is greater than s1");
}
