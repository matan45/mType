// TestSuite — marker base class for mtest test classes. Subclass it so the
// runner knows a class is a suite; hook methods are still discovered via
// @BeforeEach / @AfterEach / @BeforeAll / @AfterAll annotations, not by name.
// The empty no-arg constructor lets TestRunner instantiate subclasses
// reflectively (fresh instance per test).
public class TestSuite {
    public constructor() { }
}
