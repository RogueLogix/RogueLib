#RogueLib
Oh hey, someone actually reading this.

This is a library I use in my own projects, it is a wrapper on some other library that does the systems level stuff. 
For the most part, its on top of Boost or the C++ standard library, some sections (scripting) use other external libraries, because they have to.
Each library should have its dependencies listed below, including other parts of RogueLib.


## All
All libraries use nested namespace declaration, further C++ revision notes assume this feature is enabled

These notes also only apply to the exact version of this commit, *and may not be correct*  
Unless otherwise noted, libraries are compiled with C++17  
Headers can be used from project compiled using lower C++ revision so long as it is listed as supported

For further library details, see folder

## Exceptions
C++11, 14, 17

Just the stdlib, does make use of some GCC/Clang specific behavior, should work fine on MSVC.

## Logging
C++11, 14, 17

Again just the stdlib, uses filesystem or experimental/filesystem (checks for c++17), doesnt use boost::filesystem even if available

## Networking
C++17

Relies on RogueLib_ROBN, RogueLib_Threading, RogueLib_Logging, and RogueLib_Exceptions

## ROBN
C++17

Relies on RogueLib_Exceptions

## Scripting
##### Currently not buildable with C++20 due to a bug in llvm-clang
C++17

Relies on LLVM-Clang, RogueLib_Exceptions, RogueLib_Logging, RogueLib_Networking, RogueLib_ROBN, and RogueLib_Threading

## Threading
C++11, 14, 17

Relies on RogueLib_Exceptions and RogueLib_Logging