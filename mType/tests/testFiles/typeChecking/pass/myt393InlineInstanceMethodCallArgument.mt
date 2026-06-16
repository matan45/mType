// MYT-393: inline instance-method call results must keep their declared
// return type when used directly as another method's argument.

class Mt393Matrix {
    private function argRet(int x): int {
        return x;
    }

    private function noArgRet(): int {
        return 7;
    }

    private function sink(int v): void {
        print(v);
    }

    public function cases(Mt393Matrix other): void {
        this.sink(this.argRet(1));
        this.sink(this.noArgRet());
        this.sink(other.argRet(2));
        this.sink(other.noArgRet());
    }

    public function forwardCase(): void {
        this.sink(this.forwardRet(9));
    }

    private function forwardRet(int x): int {
        return x + 1;
    }
}

Mt393Matrix first = new Mt393Matrix();
Mt393Matrix second = new Mt393Matrix();

first.cases(second);
first.forwardCase();

// Expected output:
// 1
// 7
// 2
// 7
// 10
