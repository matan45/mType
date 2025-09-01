// Test corrupted state scenarios
class StateMachine {
    string state;
    int stateValue;
    final string INITIAL_STATE = "INIT";
    
    constructor() {
        state = INITIAL_STATE;
        stateValue = 0;
    }
    
    function transition(string newState): bool {
        if (state == "CORRUPTED") {
            return false; // Can't transition from corrupted state
        }
        
        if (newState == "CORRUPTED") {
            state = "CORRUPTED";
            stateValue = -1;
            return true;
        }
        
        state = newState;
        stateValue = stateValue + 1;
        return true;
    }
    
    function recover(): bool {
        if (state == "CORRUPTED") {
            state = INITIAL_STATE;
            stateValue = 0;
            return true;
        }
        return false;
    }
    
    function getState(): string {
        return state + ":" + toString(stateValue);
    }
}

// Test state corruption and recovery
StateMachine machine = new StateMachine();
print(machine.getState());

bool success1 = machine.transition("RUNNING");
print(success1);
print(machine.getState());

bool success2 = machine.transition("CORRUPTED");
print(success2);
print(machine.getState());

bool success3 = machine.transition("RUNNING"); // Should fail
print(success3);

bool recovered = machine.recover();
print(recovered);
print(machine.getState());