//
// DLL Entry Point Header
// Include this in main test executable to force DLL loading
//

#ifndef CONIO_UNITTEST_DLL_ENTRY_HH
#define CONIO_UNITTEST_DLL_ENTRY_HH

// Force DLL import
#if defined(_WIN32) || defined(__CYGWIN__)
    #define DLL_IMPORT __declspec(dllimport)
#else
    #define DLL_IMPORT
#endif

// This function must be called from main() to force the conio_unittest DLL
// to load and run its static test registrations
extern "C" DLL_IMPORT void conio_unittest_force_link();

#endif // CONIO_UNITTEST_DLL_ENTRY_HH
