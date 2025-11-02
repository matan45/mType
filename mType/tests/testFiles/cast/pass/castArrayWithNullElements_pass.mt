// Test array casting with mixed null and non-null elements
// Demonstrates safe handling of null values during type casting

class Entity {
    int id;
}

class Player extends Entity {
    string name;

    Player(int playerId, string playerName) {
        id = playerId;
        name = playerName;
    }
}

@Script
void testArrayWithNullElements() {
    print("Testing array casting with null elements");

    // Create array with some null elements
    Player[] players = new Player[5];
    players[0] = new Player(1, "Alice");
    players[1] = null;  // Null element
    players[2] = new Player(3, "Bob");
    players[3] = null;  // Null element
    players[4] = new Player(5, "Charlie");

    print("Array length: " + players.length);

    // Upcast to Entity array
    Entity[] entities = players;

    // Iterate and check for null
    for (int i = 0; i < entities.length; i = i + 1) {
        if (entities[i] == null) {
            print("Entity at index " + i + ": null");
        } else {
            print("Entity at index " + i + " id: " + entities[i].id);
        }
    }

    // Safe downcast of non-null elements
    if (entities[0] != null) {
        Player p = (Player)entities[0];
        print("Player 0 name: " + p.name);
    }

    if (entities[2] != null) {
        Player p = (Player)entities[2];
        print("Player 2 name: " + p.name);
    }

    if (entities[4] != null) {
        Player p = (Player)entities[4];
        print("Player 4 name: " + p.name);
    }

    print("Array with null elements casting completed");
}
