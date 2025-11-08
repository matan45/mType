// Test: Import and use nested generic types - Container<List<T>>
import * from "../../lib/primitives/Int.mt";
import { Container, Wrapper } from "modules/NestedGenericsModule.mt";

Container<Int> innerContainer = new Container<Int>(42);
Wrapper<Int> wrapper = new Wrapper<Int>(innerContainer);
Container<Int> unwrapped = wrapper.unwrap();
print(unwrapped.getItem().toString());
