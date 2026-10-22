// SwiftPM has no per-file compiler flags, so this wrapper compiles the miniaudio
// implementation as Objective-C++ (CocoaPods uses `-x objective-c++` instead).
#include <audioapi/utils/MiniaudioImplementation.cpp>
