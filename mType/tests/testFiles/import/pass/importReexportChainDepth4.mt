// Edge: import goes through a 4-deep re-export chain (ModB -> ModC -> ModD).
// The resolver recurses through each link; this guards against stack-depth
// or "isBeingEvaluated" mis-marking in deep chains.
import { ChainTip } from "./reexportChain/ModB.mt";

function main(): void {
    ChainTip tip = new ChainTip();
    print(tip.hello());
}

main();
