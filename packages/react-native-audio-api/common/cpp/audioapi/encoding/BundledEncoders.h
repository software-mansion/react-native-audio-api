#pragma once

/// Whether the Ogg/Opus encoder and remux built on the bundled libopus and libogg are compiled in.
///
/// Those libraries ship as the static external libs, so opting out of them
/// (RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED) drops them as well. A build that links
/// libopus and libogg by other means (the desktop test build against the prebuilt macOS
/// archives) defines this macro itself.
#ifndef RN_AUDIO_API_BUNDLED_OGG_OPUS
#if RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED
#define RN_AUDIO_API_BUNDLED_OGG_OPUS 0
#else
#define RN_AUDIO_API_BUNDLED_OGG_OPUS 1
#endif
#endif
