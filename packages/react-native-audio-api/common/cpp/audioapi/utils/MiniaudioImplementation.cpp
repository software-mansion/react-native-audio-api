/// Miniaudio implementation
/// this define tells the miniaudio to also include the definitions and not only declarations.
/// Which should be done only once in the whole project.
/// This make its safe to include the header file in multiple places, without causing multiple definition errors.
#define MINIAUDIO_IMPLEMENTATION
#define MA_DEBUG_OUTPUT

// MiniAudio only decodes: Ogg Vorbis and Opus through the bundled libvorbis/libopus backends,
// plus MP3 and WAV. Android tries it before MediaCodec; iOS keeps it behind the OS decoder.
#define MA_NO_FLAC
#define MA_NO_ENCODING
#define MA_NO_DEVICE_IO
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#define MA_NO_GENERATION

#include <audioapi/libs/miniaudio/miniaudio.h>
