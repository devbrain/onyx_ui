//
// Created by igor on 08/10/2025.
//

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

// Force loading of backend test DLLs
// This is required for doctest's DLL mechanism to discover tests
#ifdef ONYX_UI_BUILD_BACKEND_CONIO
    extern "C" void conio_unittest_force_link();
#endif

int main(int argc, char** argv) {
    // Force backend test DLLs to load by calling a function from each
    // This triggers static test registrations in the DLLs
#ifdef ONYX_UI_BUILD_BACKEND_CONIO
    conio_unittest_force_link();
#endif

    // Run all tests (including those registered from DLLs)
    doctest::Context context;
    context.applyCommandLine(argc, argv);

    int res = context.run();

    if (context.shouldExit()) {
        return res;
    }

    return res;
}