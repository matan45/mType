// MYT-352 follow-up regression: scope-stack cap overflow desync.
//
// 9 nested anonymous blocks each contain a stack-promoted Point alloc, so
// the compile-time STACK_SCOPE_ENTER depth reaches 9 — past the runtime
// CallFrame::kStackObjectScopeStackCap (8). Pre-fix, the compiler emits
// ENTER/LEAVE for the deepest block unconditionally; runtime silently drops
// the over-cap ENTER but the matching LEAVE still pops the outer slot, so
// d8 is prematurely returned to the ObjectInstancePool. A subsequent
// allocation in the same scope then reuses d8's slot — aliasing d8 with a
// freshly constructed Point.
//
// Post-fix (compile-time cap guard in StatementCompiler::compileBlock):
// the depth-9 block opts out of ENTER/LEAVE emission entirely, so the
// runtime scope stack stays consistent and d8 survives until block-8 LEAVE.

class Point {
    public int x;
    public int y;
    public constructor(int a, int b) {
        this.x = a;
        this.y = b;
    }
}

{                                       // depth 1
    Point d1 = new Point(1, 1);
    {                                   // depth 2
        Point d2 = new Point(2, 2);
        {                               // depth 3
            Point d3 = new Point(3, 3);
            {                           // depth 4
                Point d4 = new Point(4, 4);
                {                       // depth 5
                    Point d5 = new Point(5, 5);
                    {                   // depth 6
                        Point d6 = new Point(6, 6);
                        {               // depth 7
                            Point d7 = new Point(7, 7);
                            {           // depth 8 (at cap — last legal ENTER)
                                Point d8 = new Point(8, 8);
                                {       // depth 9 (over cap — bug zone pre-fix)
                                    Point d9 = new Point(9, 9);
                                    print("d9.x=" + d9.x);
                                }
                                // Allocate fresh Points after the over-cap
                                // block exits. Pre-fix: d8 has been released
                                // back to the pool by the desynchronised
                                // LEAVE, so one of these will alias d8 and
                                // d8.x will read the fresh constructor's
                                // value (701 or 801) instead of 8.
                                Point reuse1 = new Point(701, 702);
                                Point reuse2 = new Point(801, 802);
                                print("d8.x=" + d8.x);
                                print("reuse1.x=" + reuse1.x);
                                print("reuse2.x=" + reuse2.x);
                            }
                            print("d7.x=" + d7.x);
                        }
                        print("d6.x=" + d6.x);
                    }
                    print("d5.x=" + d5.x);
                }
                print("d4.x=" + d4.x);
            }
            print("d3.x=" + d3.x);
        }
        print("d2.x=" + d2.x);
    }
    print("d1.x=" + d1.x);
}
