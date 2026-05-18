// Round-trip setAccessible(true) then back to false. reflectPrivateField.mt
// and reflectionInvocation.mt only exercise the one-way enable. This pins
// that the flip is honored in both directions:
//   - isAccessible() reports the current state
//   - unlock + read works
//   - relock turns isAccessible back to false
// The denied-after-relock case is left to the existing privateMethodAccess
// error test, since native-thrown errors are not catchable from mType.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Field.mt";

class Vault {
    private int secret;
    public constructor() {
        this.secret = 99;
    }
}

Vault v = new Vault();
Class cls = Class::forName("Vault");
Field secretField = cls.getDeclaredField("secret");

print("initial isAccessible=" + secretField.isAccessible());

secretField.setAccessible(true);
print("after unlock isAccessible=" + secretField.isAccessible());

Object visible = secretField.get(v);
print("unlocked read=" + visible);

secretField.setAccessible(false);
print("after relock isAccessible=" + secretField.isAccessible());

secretField.setAccessible(true);
print("after second unlock isAccessible=" + secretField.isAccessible());

print("Test passed");
