// ERROR: Static vs instance method conflicts

class Parent {
    public static function process(): string {
        return "Parent static";
    }
	
	public static function process(): string {
        return "Parent";
    }
}

class Child extends Parent {
    // ERROR: Cannot override static method with instance method
    public function process(): string {
        return "Child instance";
    }
}

function main(): void {
    Child child = new Child();
    print("This should not execute");
}

main();
