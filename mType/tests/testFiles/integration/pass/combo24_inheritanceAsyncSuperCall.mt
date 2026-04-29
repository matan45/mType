// Combo 24: Inheritance + Async + super.fetch()
// Tests: abstract base async returns default; subclass overrides + awaits super

import * from "../../lib/primitives/String.mt";

abstract class BaseService {
    protected string serviceName;

    public constructor(string name) {
        this.serviceName = name;
    }

    public function async fetch(): Promise<String> {
        return new String("base:" + this.serviceName);
    }

    public function getName(): string {
        return this.serviceName;
    }
}

class UserService extends BaseService {
    public constructor() : super("user") {}

    public function async fetch(): Promise<String> {
        String parent = await super.fetch();
        return new String(parent.getValue() + " | child:user-data");
    }
}

class OrderService extends BaseService {
    public constructor() : super("order") {}

    public function async fetch(): Promise<String> {
        String parent = await super.fetch();
        return new String(parent.getValue() + " | child:order-data");
    }
}

function async main(): Promise<void> {
    print("=== Combo 24: Inheritance + Async + Super ===");

    print("--- UserService ---");
    UserService us = new UserService();
    String r1 = await us.fetch();
    print(r1.getValue());

    print("--- OrderService ---");
    OrderService os = new OrderService();
    String r2 = await os.fetch();
    print(r2.getValue());

    print("--- Names ---");
    print("user: " + us.getName());
    print("order: " + os.getName());

    print("=== Combo 24 Complete ===");
}

main();
