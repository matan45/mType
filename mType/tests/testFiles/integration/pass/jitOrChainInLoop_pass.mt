// MYT-291: asmjit's cc.finalize() crashes 0xC0000005 when the function-
// level JIT compiles a regular loop (LOOP_START + JUMP_BACK) whose body
// contains a short-circuit OR/AND chain of three or more
// JUMP_IF_*_OR_POPs feeding an IF. The IR survives emitFunctionBody;
// the crash is inside asmjit's own register allocator / assembler pass
// on the loop back-edge, when the back-edge intersects the cc.invoke
// clobber regions of the chained string-equality helpers.
//
// Reproduces in examples/code-editor's JsonCursor::skipWs:
//   while (!this.eof()) {
//     string c = this.peek();
//     if (c == " " || c == "\t" || c == "\n" || c == "\r") {
//       this.advance();
//     } else { return; }
//   }
//
// Workaround landed: JitCompiler::canCompile bails on functions matching
// LOOP_START + JUMP_BACK + 3+ JUMP_IF_*_OR_POPs. With the workaround the
// program below runs cleanly under JIT; without it, the JIT-compiled
// body crashes once skipWs-shape methods cross the 100-call hot threshold.
//
// EXPECTED:
//   ok
//
// ACTUAL (broken, before workaround):
//   FATAL: structured exception 0xc0000005 ... — likely JIT-emitted code
//   dereferenced freed/invalid memory.
//
// This file stays in bugs/ until the underlying JIT codegen issue is
// fixed (separate ticket — root cause is in asmjit's reg-alloc, not in
// canCompile). Once fixed, drop the canCompile heuristic and migrate
// this test to controlFlow/pass/ with the same content.

class Cursor {
    public string text;
    public int pos;

    public constructor(string t) {
        this.text = t;
        this.pos = 0;
    }

    public function eof(): bool {
        return this.pos >= strLength(this.text);
    }

    public function peek(): string {
        if (this.eof()) return "";
        return substring(this.text, this.pos, 1);
    }

    public function advance(): void {
        this.pos = this.pos + 1;
    }

    // The shape that crashes asmjit's cc.finalize: a while loop whose body
    // contains an `if` with a 4-way short-circuit OR chain of string
    // equalities, each emitting JUMP_IF_TRUE_OR_POP.
    public function skipWhitespaceLike(): void {
        while (!this.eof()) {
            string c = this.peek();
            if (c == " " || c == "\t" || c == "\n" || c == "\r") {
                this.advance();
            } else {
                return;
            }
        }
    }
}

function main(): void {
    // Repeat enough times to cross the JIT hot-threshold (default 100).
    // After the threshold, function-level JIT compiles skipWhitespaceLike;
    // without the canCompile workaround, asmjit crashes on cc.finalize.
    for (int i = 0; i < 200; i = i + 1) {
        Cursor cur = new Cursor("    \t\n\r  hello");
        cur.skipWhitespaceLike();
    }
    print("ok");
}

main();
