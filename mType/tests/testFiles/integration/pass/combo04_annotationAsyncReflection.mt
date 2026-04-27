// Combo 04: Annotation + Async + Reflection
// Tests: Annotated async methods discovered and invoked via reflection

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";
import * from "../../lib/reflect/Method.mt";
import * from "../../lib/primitives/String.mt";

@Retention(RUNTIME)
@Target([METHOD])
annotation Scheduled {
    int intervalMs = 1000;
    string name = "";
}

@Retention(RUNTIME)
@Target([METHOD])
annotation Cacheable {
    int ttlSeconds = 60;
}

class DataService {
    private int callCount;

    public constructor() {
        this.callCount = 0;
    }

    @Scheduled(intervalMs = 5000, name = "fetchData")
    public function async fetchData(): Promise<String> {
        this.callCount = this.callCount + 1;
        return new String("data-" + this.callCount);
    }

    @Scheduled(intervalMs = 10000, name = "cleanup")
    @Cacheable(ttlSeconds = 30)
    public function async cleanup(): Promise<String> {
        return new String("cleaned");
    }

    public function getCallCount(): int {
        return this.callCount;
    }

    public function sync(): string {
        return "sync-result";
    }
}

function discoverScheduledMethods(string className): void {
    Class cls = Class::forName(className);
    print("Inspecting class: " + cls.getName());

    Method[] methods = cls.getDeclaredMethods();
    print("Total methods: " + methods.length);

    for (int i = 0; i < methods.length; i++) {
        Method m = methods[i];
        if (m.hasAnnotation("Scheduled")) {
            Annotation? ann = m.getAnnotation("Scheduled");
            if (ann != null) {
                string name = ann.getString("name");
                int interval = ann.getInt("intervalMs");
                print("Found scheduled: " + name + " (every " + interval + "ms)");

                if (m.hasAnnotation("Cacheable")) {
                    Annotation? cache = m.getAnnotation("Cacheable");
                    if (cache != null) {
                        int ttl = cache.getInt("ttlSeconds");
                        print("  -> Cacheable with TTL: " + ttl + "s");
                    }
                }
            }
        }
    }
}

function async main(): Promise<void> {
    print("=== Combo 04: Annotation + Async + Reflection ===");

    print("--- Direct async calls ---");
    DataService service = new DataService();
    String r1 = await service.fetchData();
    print("Fetch 1: " + r1.getValue());
    String r2 = await service.fetchData();
    print("Fetch 2: " + r2.getValue());
    String r3 = await service.cleanup();
    print("Cleanup: " + r3.getValue());
    print("Call count: " + service.getCallCount());

    print("--- Reflection discovery ---");
    discoverScheduledMethods("DataService");

    print("=== Combo 04 Complete ===");
}

main();
