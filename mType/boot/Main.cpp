#pragma once
#include "../tests/runtime/LexerTest.hpp"

using namespace tests::runtime;

int main(int argc, char* argv[])
{
    LexerTest tester;
    tester.setVerbose(true);
    tester.runAllTests();
}
