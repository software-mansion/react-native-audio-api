#include <audioapi/HostObjects/utils/JsEnumParser.h>
#include <string>

using WindowType = audioapi::AnalyserNode::WindowType;

namespace audioapi::js_enum_parser {

  WindowType windowTypeFromString(const std::string &type) {
    if (type == "blackman") {
      return WindowType::BLACKMAN;
    }
    if (type == "hann") {
      return WindowType::HANN;
    }

    throw std::invalid_argument("Unknown window type");
  }


  std::string windowTypeToString(WindowType type) {
    switch (type) {
      case WindowType::BLACKMAN:
        return "blackman";
      case WindowType::HANN:
        return "hann";
      default:
        throw std::invalid_argument("Unknown window type");
    }
  }

  BiquadFilterType filterTypeFromString(const std::string &type) {
    if (type == "lowpass")
      return BiquadFilterType::LOWPASS;
    if (type == "highpass")
      return BiquadFilterType::HIGHPASS;
    if (type == "bandpass")
      return BiquadFilterType::BANDPASS;
    if (type == "lowshelf")
      return BiquadFilterType::LOWSHELF;
    if (type == "highshelf")
      return BiquadFilterType::HIGHSHELF;
    if (type == "peaking")
      return BiquadFilterType::PEAKING;
    if (type == "notch")
      return BiquadFilterType::NOTCH;
    if (type == "allpass")
      return BiquadFilterType::ALLPASS;

    throw std::invalid_argument("Invalid filter type: " + type);
  }

  std::string filterTypeToString(BiquadFilterType type) {
    switch (type) {
      case BiquadFilterType::LOWPASS:
        return "lowpass";
      case BiquadFilterType::HIGHPASS:
        return "highpass";
      case BiquadFilterType::BANDPASS:
        return "bandpass";
      case BiquadFilterType::LOWSHELF:
        return "lowshelf";
      case BiquadFilterType::HIGHSHELF:
        return "highshelf";
      case BiquadFilterType::PEAKING:
        return "peaking";
      case BiquadFilterType::NOTCH:
        return "notch";
      case BiquadFilterType::ALLPASS:
        return "allpass";
      default:
        throw std::invalid_argument("Unknown filter type");
    }
  }

  OverSampleType overSampleTypeFromString(const std::string &type) {
    if (type == "2x")
      return OverSampleType::OVERSAMPLE_2X;
    if (type == "4x")
      return OverSampleType::OVERSAMPLE_4X;

    return OverSampleType::OVERSAMPLE_NONE;
  }

  std::string overSampleTypeToString(OverSampleType type) {
    switch (type) {
      case OverSampleType::OVERSAMPLE_2X:
        return "2x";
      case OverSampleType::OVERSAMPLE_4X:
        return "4x";
      default:
        return "none";
    }
  }

  OscillatorType oscillatorTypeFromString(const std::string &type) {
    if (type == "sine")
      return OscillatorType::SINE;
    if (type == "square")
      return OscillatorType::SQUARE;
    if (type == "sawtooth")
      return OscillatorType::SAWTOOTH;
    if (type == "triangle")
      return OscillatorType::TRIANGLE;
    if (type == "custom")
      return OscillatorType::CUSTOM;

    throw std::invalid_argument("Unknown oscillator type: " + type);
  }

  std::string oscillatorTypeToString(OscillatorType type) {
    switch (type) {
      case OscillatorType::SINE:
        return "sine";
      case OscillatorType::SQUARE:
        return "square";
      case OscillatorType::SAWTOOTH:
        return "sawtooth";
      case OscillatorType::TRIANGLE:
        return "triangle";
      case OscillatorType::CUSTOM:
        return "custom";
      default:
        throw std::invalid_argument("Unknown oscillator type");
    }
  }

  AudioEvent audioEventFromString(const std::string &event) {
    if (event == "playbackNotificationPlay")
        return AudioEvent::PLAYBACK_NOTIFICATION_PLAY;
    if (event == "playbackNotificationPause")
        return AudioEvent::PLAYBACK_NOTIFICATION_PAUSE;
    if (event == "playbackNotificationStop")
        return AudioEvent::PLAYBACK_NOTIFICATION_STOP;
    if (event == "playbackNotificationNextTrack")
        return AudioEvent::PLAYBACK_NOTIFICATION_NEXT_TRACK;
    if (event == "playbackNotificationPreviousTrack")
        return AudioEvent::PLAYBACK_NOTIFICATION_PREVIOUS_TRACK;
    if (event == "playbackNotificationSkipForward")
        return AudioEvent::PLAYBACK_NOTIFICATION_SKIP_FORWARD;
    if (event == "playbackNotificationSkipBackward")
        return AudioEvent::PLAYBACK_NOTIFICATION_SKIP_BACKWARD;
    if (event == "playbackNotificationSeekForward")
        return AudioEvent::PLAYBACK_NOTIFICATION_SEEK_FORWARD;
    if (event == "playbackNotificationSeekBackward")
        return AudioEvent::PLAYBACK_NOTIFICATION_SEEK_BACKWARD;
    if (event == "playbackNotificationSeekTo")
        return AudioEvent::PLAYBACK_NOTIFICATION_SEEK_TO;
    if (event == "playbackNotificationDismissed")
        return AudioEvent::PLAYBACK_NOTIFICATION_DISMISSED;
    if (event == "recordingNotificationResume")
        return AudioEvent::RECORDING_NOTIFICATION_RESUME;
    if (event == "recordingNotificationPause")
        return AudioEvent::RECORDING_NOTIFICATION_PAUSE;
    if (event == "routeChange")
        return AudioEvent::ROUTE_CHANGE;
    if (event == "interruption")
        return AudioEvent::INTERRUPTION;
    if (event == "volumeChange")
        return AudioEvent::VOLUME_CHANGE;
    if (event == "duck")
        return AudioEvent::DUCK;
    if (event == "ended")
        return AudioEvent::ENDED;
    if (event == "loopEnded")
      return AudioEvent::LOOP_ENDED;
    if (event == "audioReady")
      return AudioEvent::AUDIO_READY;
    if (event == "positionChanged")
      return AudioEvent::POSITION_CHANGED;
    if (event == "bufferEnded")
      return AudioEvent::BUFFER_ENDED;
    if (event == "recorderError")
      return AudioEvent::RECORDER_ERROR;

    throw std::invalid_argument("Unknown audio event: " + event);
  }

  std::string contextStateToString(ContextState state) {
    switch (state) {
      case ContextState::SUSPENDED:
        return "suspended";
      case ContextState::RUNNING:
        return "running";
      case ContextState::CLOSED:
        return "closed";
      default:
        throw std::invalid_argument("Unknown context state");
    }
  }
} // namespace audioapi::js_enum_parser
