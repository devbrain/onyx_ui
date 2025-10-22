//
// DLL Entry Point for Test Registration
// This function must be called from the main executable to force the DLL
// to load and run its static initializers, which register all tests.
//

// Force DLL export
#if defined(_WIN32) || defined(__CYGWIN__)
    #define DLL_EXPORT __declspec(dllexport)
#else
    #define DLL_EXPORT __attribute__((visibility("default")))
#endif

// This function forces the DLL to be loaded and its static test registrations to execute
extern "C" DLL_EXPORT void conio_unittest_force_link() {
    // This function intentionally does nothing
    // Its purpose is to be called from main to force the linker to load this DLL
    // When the DLL loads, all static test registrations will automatically execute
}
