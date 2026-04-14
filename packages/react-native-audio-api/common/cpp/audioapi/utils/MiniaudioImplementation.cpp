/// Miniaudio implementation
/// this define tells the miniaudio to also include the definitions and not only declarations.
/// Which should be done only once in the whole project.
/// This make its safe to include the header file in multiple places, without causing multiple definition errors.
#define MINIAUDIO_IMPLEMENTATION
#define MA_DEBUG_OUTPUT
#include <audioapi/libs/miniaudio/miniaudio.h>
