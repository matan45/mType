// Test: Import and use Random from lib/math
import { Random } from "../../lib/math/Random.mt";

bool intsInRange = true;
bool floatsInRange = true;

for (int i = 0; i < 20; i = i + 1) {
    int value = Random::nextInt(1, 4);
    if (value < 1 || value >= 4) {
        intsInRange = false;
    }
}

for (int i = 0; i < 20; i = i + 1) {
    float value = Random::nextFloat(0.5, 1.5);
    if (value < 0.5 || value >= 1.5) {
        floatsInRange = false;
    }
}

print(intsInRange);
print(floatsInRange);
