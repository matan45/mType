// Test 21: Data and Control Flow Edge Cases
// Features: Value classes, String Interpolation, String Pool, Switch case default

value class DataPacket {
    private string id;
    private string payload;

    public constructor(string id, string payload) {
        this.id = id;
        this.payload = payload;
    }

    public function getId(): string {
        return this.id;
    }

    public function getPayload(): string {
        return this.payload;
    }
    
    public function format(): string {
        return $"Packet[{this.id}]: {this.payload}";
    }
}

function processPacket(DataPacket packet): string {
    string result = "";
    string cmd = packet.getId();
    
    // Switch on string tests string pooling/equality under the hood
    switch (cmd) {
        case "INIT":
            result = $"Initialization started with payload: {packet.getPayload()}";
            break;
        case "DATA":
            result = $"Processing data chunk: {packet.getPayload()}";
            break;
        case "END":
            result = $"Finalizing with: {packet.getPayload()}";
            break;
        default:
            result = $"Unknown command '{cmd}' containing '{packet.getPayload()}'";
            break;
    }
    
    return result;
}

function main(): void {
    print("--- Test 21: Data and Control Flow Edge Cases ---");
    
    // Force string pool usage by constructing strings that match cases
    string cmd1 = "IN" + "IT"; 
    string cmd2 = "DA" + "TA";
    string cmd3 = "E" + "ND";
    string cmd4 = "ERR" + "OR";
    
    DataPacket p1 = new DataPacket(cmd1, "System Boot");
    DataPacket p2 = new DataPacket(cmd2, "0xDEADBEEF");
    DataPacket p3 = new DataPacket(cmd3, "System Halt");
    DataPacket p4 = new DataPacket(cmd4, "Core Dump");
    
    print(p1.format());
    print(processPacket(p1));
    
    print(p2.format());
    print(processPacket(p2));
    
    print(p3.format());
    print(processPacket(p3));
    
    print(p4.format());
    print(processPacket(p4));
    
    print("--- String Pool Validation ---");
    string literalInit = "INIT";
    if (cmd1 == literalInit) {
        print("String pool matched concatenated string to literal!");
    } else {
        print("String pool match failed.");
    }
}

main();