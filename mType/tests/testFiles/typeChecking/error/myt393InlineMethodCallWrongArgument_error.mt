// MYT-393: method-call return inference must still reject a wrong typed
// argument on the real return type, not hide it behind void.

class Mt393WrongArgument {
    private function stringValue(): string {
        return "wrong";
    }

    private function sink(int v): void {
        print(v);
    }

    public function trigger(): void {
        this.sink(this.stringValue());
    }
}

Mt393WrongArgument repro = new Mt393WrongArgument();
repro.trigger();
