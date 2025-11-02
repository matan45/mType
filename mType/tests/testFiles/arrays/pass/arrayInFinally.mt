// Test arrays in finally-like cleanup blocks
print("Testing arrays in cleanup blocks");

class ResourceManager {
    int[] resources;

    ResourceManager() {
        resources = new int[0];
    }

    void acquire(int count) {
        resources = new int[count];
        for (int i = 0; i < resources.length; i++) {
            resources[i] = i + 1;
        }
    }

    void release() {
        print("Releasing " + resources.length + " resources");
        resources = new int[0];
    }

    int[] getResources() {
        return resources;
    }
}

ResourceManager manager = new ResourceManager();

// Try block equivalent
manager.acquire(5);
print("Acquired resources:");
int[] acquired = manager.getResources();
for (int i = 0; i < acquired.length; i++) {
    print("  Resource " + acquired[i]);
}

// Finally block equivalent
manager.release();

print("After cleanup, resources count: " + manager.getResources().length);

print("Cleanup blocks test completed");
