import * from "../../mType/tests/testFiles/lib/exceptions/Exception.mt";
import * from "../../mType/tests/testFiles/lib/net/TcpSocket.mt";
import * from "../../mType/tests/testFiles/lib/primitives/Int.mt";
import * from "../../mType/tests/testFiles/lib/primitives/String.mt";

function async receiveOne(string label, TcpSocket socket): Promise<void> {
    try {
        String received = await socket.receiveAsync(2048);
        if (received.getValue() != "") {
            print("[" + label + "] recv: " + received.getValue());
        }
    } catch (Exception e) {
        print("[" + label + "] receive timeout");
    }
}

function async sendChat(string label, TcpSocket socket, string message): Promise<void> {
    print("[" + label + "] send: " + message);
    Int sent = await socket.sendAsync(message);
}

function async main(): Promise<void> {
    TcpSocket alice = new TcpSocket();
    TcpSocket bob = new TcpSocket();
    alice.setTimeout(5000);
    bob.setTimeout(5000);

    try {
        await alice.connectAsync("127.0.0.1", 18888);
        print("[alice] connected");
        await sendChat("alice", alice, "alice");
        await delay(100);
        await receiveOne("alice", alice);

        await bob.connectAsync("127.0.0.1", 18888);
        print("[bob] connected");
        await sendChat("bob", bob, "bob");
        await delay(100);
        await receiveOne("alice", alice);
        await receiveOne("bob", bob);

        await sendChat("alice", alice, "hello from alice");
        await delay(100);
        await receiveOne("alice", alice);
        await receiveOne("bob", bob);

        await sendChat("bob", bob, "hi alice");
        await delay(100);
        await receiveOne("alice", alice);
        await receiveOne("bob", bob);

        await sendChat("alice", alice, "/quit");
        await delay(100);
        await receiveOne("bob", bob);

        await sendChat("bob", bob, "/quit");
        await delay(100);
    } catch (Exception e) {
        print("[client] error: " + e.getMessage());
    }

    alice.close();
    bob.close();
    print("[client] scripted chat complete");
}

main();
