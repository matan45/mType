// Test interface with excessive generic parameters (21 parameters)
// This should test system limits and potentially trigger depth/complexity limits

interface ExcessiveProcessor<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21> {
    function process1(T1 p1): T1;
    function process2(T2 p2): T2;
    function process3(T3 p3): T3;
    function process4(T4 p4): T4;
    function process5(T5 p5): T5;
    function process6(T6 p6): T6;
    function process7(T7 p7): T7;
    function process8(T8 p8): T8;
    function process9(T9 p9): T9;
    function process10(T10 p10): T10;
    function process11(T11 p11): T11;
    function process12(T12 p12): T12;
    function process13(T13 p13): T13;
    function process14(T14 p14): T14;
    function process15(T15 p15): T15;
    function process16(T16 p16): T16;
    function process17(T17 p17): T17;
    function process18(T18 p18): T18;
    function process19(T19 p19): T19;
    function process20(T20 p20): T20;
    function process21(T21 p21): T21;

    function processAll(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10,
                       T11 p11, T12 p12, T13 p13, T14 p14, T15 p15, T16 p16, T17 p17, T18 p18, T19 p19, T20 p20, T21 p21): string;
}

// Create dummy classes for all 21 type parameters
class Type1 { string value; constructor(string v) { value = v; } }
class Type2 { string value; constructor(string v) { value = v; } }
class Type3 { string value; constructor(string v) { value = v; } }
class Type4 { string value; constructor(string v) { value = v; } }
class Type5 { string value; constructor(string v) { value = v; } }
class Type6 { string value; constructor(string v) { value = v; } }
class Type7 { string value; constructor(string v) { value = v; } }
class Type8 { string value; constructor(string v) { value = v; } }
class Type9 { string value; constructor(string v) { value = v; } }
class Type10 { string value; constructor(string v) { value = v; } }
class Type11 { string value; constructor(string v) { value = v; } }
class Type12 { string value; constructor(string v) { value = v; } }
class Type13 { string value; constructor(string v) { value = v; } }
class Type14 { string value; constructor(string v) { value = v; } }
class Type15 { string value; constructor(string v) { value = v; } }
class Type16 { string value; constructor(string v) { value = v; } }
class Type17 { string value; constructor(string v) { value = v; } }
class Type18 { string value; constructor(string v) { value = v; } }
class Type19 { string value; constructor(string v) { value = v; } }
class Type20 { string value; constructor(string v) { value = v; } }
class Type21 { string value; constructor(string v) { value = v; } }

// This implementation might exceed system limits
class ExcessiveProcessorImpl implements ExcessiveProcessor<Type1, Type2, Type3, Type4, Type5, Type6, Type7, Type8, Type9, Type10, Type11, Type12, Type13, Type14, Type15, Type16, Type17, Type18, Type19, Type20, Type21> {
    function process1(Type1 p1): Type1 { return p1; }
    function process2(Type2 p2): Type2 { return p2; }
    function process3(Type3 p3): Type3 { return p3; }
    function process4(Type4 p4): Type4 { return p4; }
    function process5(Type5 p5): Type5 { return p5; }
    function process6(Type6 p6): Type6 { return p6; }
    function process7(Type7 p7): Type7 { return p7; }
    function process8(Type8 p8): Type8 { return p8; }
    function process9(Type9 p9): Type9 { return p9; }
    function process10(Type10 p10): Type10 { return p10; }
    function process11(Type11 p11): Type11 { return p11; }
    function process12(Type12 p12): Type12 { return p12; }
    function process13(Type13 p13): Type13 { return p13; }
    function process14(Type14 p14): Type14 { return p14; }
    function process15(Type15 p15): Type15 { return p15; }
    function process16(Type16 p16): Type16 { return p16; }
    function process17(Type17 p17): Type17 { return p17; }
    function process18(Type18 p18): Type18 { return p18; }
    function process19(Type19 p19): Type19 { return p19; }
    function process20(Type20 p20): Type20 { return p20; }
    function process21(Type21 p21): Type21 { return p21; }

    function processAll(Type1 p1, Type2 p2, Type3 p3, Type4 p4, Type5 p5, Type6 p6, Type7 p7, Type8 p8, Type9 p9, Type10 p10,
                       Type11 p11, Type12 p12, Type13 p13, Type14 p14, Type15 p15, Type16 p16, Type17 p17, Type18 p18, Type19 p19, Type20 p20, Type21 p21): string {
        return "Processed 21 parameters";
    }
}

function main(): void {
    print("Testing interface with 21 generic parameters...");

    ExcessiveProcessorImpl processor = new ExcessiveProcessorImpl();
    print("If you see this, the system handled 21 generic parameters successfully!");
}

main();