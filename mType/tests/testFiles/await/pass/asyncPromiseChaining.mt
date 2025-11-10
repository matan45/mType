// Test promise chaining where one promise creates another

import { Int } from "../../lib/primitives/Int.mt";

print("=== Promise Chaining Test ===");

class ChainLink {
    int linkNumber;

    public constructor(int num) {
        this.linkNumber = num;
    }

    public function getNumber(): int {
        return this.linkNumber;
    }
}

function async createLink1(): Promise<ChainLink> {
    print("Creating link 1");
    ChainLink link = new ChainLink(1);
    return link;
}

function async createLink2(ChainLink prev): Promise<ChainLink> {
    print("Creating link 2 from link " + prev.getNumber());
    ChainLink link = new ChainLink(2);
    return link;
}

function async createLink3(ChainLink prev): Promise<ChainLink> {
    print("Creating link 3 from link " + prev.getNumber());
    ChainLink link = new ChainLink(3);
    return link;
}

function async testChaining(): Promise<Int> {
    print("Starting chain");

    // Chain promises where each depends on previous
    ChainLink link1 = await createLink1();
    print("Got link: " + link1.getNumber());

    ChainLink link2 = await createLink2(link1);
    print("Got link: " + link2.getNumber());

    ChainLink link3 = await createLink3(link2);
    print("Got link: " + link3.getNumber());

    int sum = link1.getNumber() + link2.getNumber() + link3.getNumber();
    print("Chain sum: " + sum);

    return new Int(sum);
}

function async main(): Promise<Int> {
    Int result = await testChaining();
    print("Chain complete: " + result);
    return result;
}

main();
