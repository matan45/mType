// MYT-221 lock-in: lib/functional/Consumer.mt is now a true SAM interface.
// `Consumer<T>` declares only `accept`, so a lambda binds directly.

import * from "../../lib/functional/Consumer.mt";
import * from "../../lib/primitives/String.mt";

Consumer<String> printer = s -> print(s.getValue());
printer.accept(new String("Hello"));
