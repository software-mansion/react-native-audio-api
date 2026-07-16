#pragma once

/**
 * Stable C++ compatibility API for react-native-audio-api.
 *
 * Extension packages must include **only** this header — do not include other
 * `audioapi/...` headers directly. Symbols not reachable through this file are
 * internal and may change without notice.
 */

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/HostObjects/BaseAudioContextHostObject.h>
#include <audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h>
#include <audioapi/core/AudioNode.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/SpectrumAnalyser.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArrayBuffer.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Macros.h>
