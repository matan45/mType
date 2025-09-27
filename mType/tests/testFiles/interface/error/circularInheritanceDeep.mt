// Test that circular inheritance is detected even in deep hierarchies
// This should fail with circular dependency detection

interface IA extends IJ {
    function methodA(): void;
}

interface IB extends IA {
    function methodB(): void;
}

interface IC extends IB {
    function methodC(): void;
}

interface ID extends IC {
    function methodD(): void;
}

interface IE extends ID {
    function methodE(): void;
}

interface IF extends IE {
    function methodF(): void;
}

interface IG extends IF {
    function methodG(): void;
}

interface IH extends IG {
    function methodH(): void;
}

interface II extends IH {
    function methodI(): void;
}

interface IJ extends II {
    function methodJ(): void;
}

function main(): void {
    print("This should not execute due to circular inheritance");
}

main();