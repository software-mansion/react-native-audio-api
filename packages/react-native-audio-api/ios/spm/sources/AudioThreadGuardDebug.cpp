// AudioThreadGuard replaces the global operator new/delete, so it must stay out of
// Release builds. SwiftPM cannot exclude a file per configuration, hence the guard.
#ifdef DEBUG
#include <audioapi/utils/AudioThreadGuard.cpp>
#endif
