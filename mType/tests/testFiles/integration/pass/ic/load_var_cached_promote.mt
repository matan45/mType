// MYT-204: LOAD_VAR_CACHED correctness under stable global resolution.
// Hot loop reads a global many times; the LOAD_VAR site inside readGlobal
// gets promoted to LOAD_VAR_CACHED on the first call and the cached path
// dereferences the VariableDefinition slot directly. Output must match
// the pre-rewrite generic LOAD_VAR dispatch.

int g = 7;

function readGlobal(): int {
    return g;
}

int acc = 0;
for (int i = 0; i < 1000; i = i + 1) {
    acc = acc + readGlobal();
}
print(acc);
