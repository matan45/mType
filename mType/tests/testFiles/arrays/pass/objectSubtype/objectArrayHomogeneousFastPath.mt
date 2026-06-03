// MYT-378 regression: a homogeneous Repo[16] (every element exactly the
// declared class) must stay on the fast SoA path and round-trip its fields
// correctly. Guards against the fix accidentally always falling back to
// heterogeneous storage.
print("Testing homogeneous fast path");

class Repo {
    public int id;
    public constructor(int v) {
        id = v;
    }
    public function get(): int {
        return id;
    }
}

Repo[] repos = new Repo[16];
for (int i = 0; i < 16; i++) {
    repos[i] = new Repo(i);
}

int sum = 0;
for (int i = 0; i < 16; i++) {
    sum = sum + repos[i].get();
}

print("Sum: " + sum);
print("Done");
