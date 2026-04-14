// MYT-108: annotation applied to a top-level (non-class) function.
// The typed usage validator runs during function registration so unknown
// params or type mismatches are caught before execution.

annotation Retry {
    int times = 3;
}

@Retry(times = 5)
function doWork(): int {
    return 42;
}

int result = doWork();
print(result);
