// Test arrays with deferred operations (simulating async pattern)
print("Testing deferred array operations");

class DeferredResult {
    int[] data;
    bool ready;

    DeferredResult() {
        data = new int[0];
        ready = false;
    }

    void complete(int[] result) {
        data = result;
        ready = true;
    }

    bool isReady() {
        return ready;
    }

    int[] getData() {
        return data;
    }
}

int[] processArray(int[] input) {
    int[] result = new int[input.length];
    for (int i = 0; i < input.length; i++) {
        result[i] = input[i] * 2;
    }
    return result;
}

int[] source = new int[4];
source[0] = 1;
source[1] = 2;
source[2] = 3;
source[3] = 4;

DeferredResult deferred = new DeferredResult();
deferred.complete(processArray(source));

if (deferred.isReady()) {
    int[] result = deferred.getData();
    print("Deferred operation completed:");
    for (int i = 0; i < result.length; i++) {
        print("  result[" + i + "] = " + result[i]);
    }
}

print("Deferred operations test completed");
