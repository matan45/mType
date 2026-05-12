// MYT-302 follow-up: an OSR loop with a forward conditional region containing
// a static void call must not silently exit even when the branch is never
// taken. The second loop keeps the later alias-read shape that exposed the
// corrupted OSR state in the ticket repro.

class Unit {
    public float x;
    public bool selected;
    public constructor(float x) {
        this.x = x;
        this.selected = false;
    }
}

function consume(float x): void { }

Unit[] units = new Unit[512];
int n = 20;
for (int i = 0; i < n; i = i + 1) {
    units[i] = new Unit(i * 1.0);
}

int trueSightings = 0;
float xSum = 0.0;
for (int frame = 0; frame < 5000; frame = frame + 1) {
    for (int i = 0; i < n; i = i + 1) {
        Unit u = units[i];
        if (u.selected) {
            consume(u.x);
            trueSightings = trueSightings + 1;
        }
    }
    for (int i = 0; i < n; i = i + 1) {
        Unit u = units[i];
        xSum = xSum + u.x;
    }
}

print("conditionalVoidCallTrueSightings=" + trueSightings);
print("xSum=" + xSum);
