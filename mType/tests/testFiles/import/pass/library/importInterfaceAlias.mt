// Test: import interface with alias
import {Comparable as Cmp} from "../../../lib/interfaces/Comparable.mt";
import {Int as MyInt} from "../../../lib/primitives/Int.mt";

class Score implements Cmp<Score> {
    public int points;

    public constructor(int p) {
        this.points = p;
    }

    public function compareTo(Score other): int {
        return this.points - other.points;
    }
}

Score a = new Score(10);
Score b = new Score(20);
int result = a.compareTo(b);
if (result < 0) {
    print("a < b");
} else {
    print("a >= b");
}
print("Interface alias test passed");
