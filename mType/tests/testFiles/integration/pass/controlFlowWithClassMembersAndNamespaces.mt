namespace gameLogic {
    final int MAX_HEALTH = 100;
    final int MAX_LEVEL = 50;
    
    class Player {
        int health;
        int level;
        string name;
        final int MAX_EXPERIENCE = 1000;
        static int playerCount = 0;
        
        constructor(string playerName) {
            name = playerName;
            health = MAX_HEALTH;
            level = 1;
            playerCount = playerCount + 1;
        }
        
        function takeDamage(int damage): bool {
            health = health - damage;
            if (health <= 0) {
                health = 0;
                return false; // Player died
            }
            return true; // Player alive
        }
        
        function levelUp(): bool {
            if (level < MAX_LEVEL) {
                level = level + 1;
                health = MAX_HEALTH; // Full heal on level up
                return true;
            }
            return false;
        }
        
        function getStatus(): string {
            if (health == 0) {
                return "Dead";
            } else if (health < 20) {
                return "Critical";
            } else if (health < 50) {
                return "Wounded";
            } else {
                return "Healthy";
            }
        }
    }
    
    namespace combat {
        function simulateBattle(gameLogic::Player player1, gameLogic::Player player2): string {
            final int MAX_ROUNDS = 10;
            
            for (int round = 0; round < MAX_ROUNDS; round++) {
                // Player 1 attacks Player 2
                bool p2Alive = player2.takeDamage(15);
                if (!p2Alive) {
                    return player1.name + " wins in round " + toString(round + 1);
                }
                
                // Player 2 attacks Player 1
                bool p1Alive = player1.takeDamage(12);
                if (!p1Alive) {
                    return player2.name + " wins in round " + toString(round + 1);
                }
                
                // Check for level up conditions (simplified)
                if (round % 3 == 0 && round > 0) {
                    player1.levelUp();
                    player2.levelUp();
                }
            }
            
            // Determine winner by health
            if (player1.health > player2.health) {
                return player1.name + " wins by health";
            } else if (player2.health > player1.health) {
                return player2.name + " wins by health";
            } else {
                return "Draw";
            }
        }
    }
    
    namespace tournament {
        function runTournament(): string {
            final int NUM_PLAYERS = 4;
            string results = "";
            
            // Create players
            gameLogic::Player player1 = new gameLogic::Player("Warrior");
            gameLogic::Player player2 = new gameLogic::Player("Mage");
            gameLogic::Player player3 = new gameLogic::Player("Archer");
            gameLogic::Player player4 = new gameLogic::Player("Rogue");
            
            // Simulate matches with complex control flow
            for (int match = 0; match < 2; match++) {
                if (match == 0) {
                    string result1 = gameLogic::combat::simulateBattle(player1, player2);
                    string result2 = gameLogic::combat::simulateBattle(player3, player4);
                    results = results + "Semi 1: " + result1 + " | Semi 2: " + result2;
                } else {
                    // Final match - determine finalists based on health
                    gameLogic::Player finalist1 = player1.health > player2.health ? player1 : player2;
                    gameLogic::Player finalist2 = player3.health > player4.health ? player3 : player4;
                    
                    string finalResult = gameLogic::combat::simulateBattle(finalist1, finalist2);
                    results = results + " | Final: " + finalResult;
                }
            }
            
            return results;
        }
    }
}

// Execute complex control flow integration
gameLogic::Player testPlayer = new gameLogic::Player("TestHero");
print(testPlayer.getStatus());

bool alive = testPlayer.takeDamage(30);
print(alive);
print(testPlayer.getStatus());

bool leveled = testPlayer.levelUp();
print(leveled);
print(testPlayer.getStatus());

// Run complex tournament simulation
string tournamentResult = gameLogic::tournament::runTournament();
print(tournamentResult);
print(gameLogic::Player::playerCount);