#pragma once

namespace tests::testFramework
{
    class TestSuite;
}

namespace tests::testSuite
{
    void registerGCRuntimeCorrectnessTests(testFramework::TestSuite& suite);
}
