// Test: Import and use nested generic types - Container<List<T>>
@Script
import { Container, Wrapper } from "./modules/NestedGenericsModule.mt"

var innerContainer = Container<Int>(42);
var wrapper = Wrapper<Int>(innerContainer);
var unwrapped = wrapper.unwrap();
print(unwrapped.getItem());
