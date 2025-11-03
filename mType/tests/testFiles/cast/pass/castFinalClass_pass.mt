// Test casting involving final classes
class Base {
    public function getName(): string {
        return "Base";
    }
}

final class FinalDerived extends Base {
    public function getName(): string {
        return "FinalDerived";
    }

    public function getSpecial(): string {
        return "Special";
    }
}

function processBase(Base obj): string {
    return obj.getName();
}


function main(): void {
    FinalDerived finalDerived = new FinalDerived();
    Base base = finalDerived;  // Upcast from final class

    print(base.getName());  // Should use virtual dispatch
    print(processBase(finalDerived));  // Implicit upcast

    FinalDerived backToFinal = (FinalDerived)base;  // Downcast to final
    print(backToFinal.getSpecial());
}
main();
