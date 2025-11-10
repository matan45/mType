// Test casting with diamond inheritance pattern
// Diamond: I1 <- I2, I3; Class implements I2, I3
// Should handle casting through all paths in the diamond

interface Top {
    public function getTopValue(): string;
}

interface LeftBranch extends Top {
    public function getLeftValue(): string;
}

interface RightBranch extends Top {
    public function getRightValue(): string;
}

class DiamondImpl implements LeftBranch, RightBranch {
    public function getTopValue(): string {
        return "top";
    }

    public function getLeftValue(): string {
        return "left";
    }

    public function getRightValue(): string {
        return "right";
    }
}

// Create instance
DiamondImpl obj = new DiamondImpl();

// Cast to both branches
LeftBranch left = (LeftBranch)obj;
RightBranch right = (RightBranch)obj;

// Cast to top interface through both paths
Top topFromLeft = (Top)left;
Top topFromRight = (Top)right;

// Direct cast to top
Top topDirect = (Top)obj;

// Verify all casts work correctly
print(left.getLeftValue());
print(((Top)left).getTopValue());
print(right.getRightValue());
print(((Top)right).getTopValue());
print(topFromLeft.getTopValue());
print(topFromRight.getTopValue());
print(topDirect.getTopValue());

// Cross-cast through top
Top temp = (Top)obj;
LeftBranch leftAgain = (LeftBranch)obj;
RightBranch rightAgain = (RightBranch)obj;

print(leftAgain.getLeftValue());
print(rightAgain.getRightValue());

print("Diamond casting test passed");
