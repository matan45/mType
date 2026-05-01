// TARGET: ~1-3s on dev machine. Adjust N if first run lands outside that range.
// Exercises abstract base dispatch through overridden methods.

abstract class AbstractWorker {
    abstract function compute(int x): int;
}

class DoubleWorker extends AbstractWorker {
    public function compute(int x): int {
        return x * 2;
    }
}

class AddWorker extends AbstractWorker {
    public function compute(int x): int {
        return x + 11;
    }
}

class XorWorker extends AbstractWorker {
    public function compute(int x): int {
        return x ^ 7;
    }
}

AbstractWorker[] workers = [new DoubleWorker(), new AddWorker(), new XorWorker()];

int N = 2000000;
int acc = 0;
for (int i = 0; i < N; i = i + 1) {
    acc = acc + workers[i % 3].compute(i);
}

print("abstract_dispatch_hot acc=" + acc);
