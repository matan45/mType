@Script
// Test casting involving final classes
class Base {
    fn getName(): String {
        return "Base";
    }
}

final class FinalDerived : Base {
    fn getName(): String {
        return "FinalDerived";
    }

    fn getSpecial(): String {
        return "Special";
    }
}

fn processBase(obj: Base): String {
    return obj.getName();
}

fn main() {
    let final: FinalDerived = new FinalDerived();
    let base: Base = final as Base;  // Upcast from final class

    print(base.getName());  // Should use virtual dispatch
    print(processBase(final));  // Implicit upcast

    let backToFinal: FinalDerived = base as FinalDerived;  // Downcast to final
    print(backToFinal.getSpecial());
}
