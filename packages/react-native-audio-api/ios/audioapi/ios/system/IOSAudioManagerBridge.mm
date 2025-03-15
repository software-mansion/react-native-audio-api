#import <AVFoundation/AVFoundation.h>

#include <audioapi/ios/system/IOSAudioManagerBridge.h>
#include <audioapi/system/SessionOptions.h>

namespace audioapi {

IOSAudioManagerBridge::IOSAudioManagerBridge()
{
  iosAudioManager_ = [[IOSAudioManager alloc] init];
}

IOSAudioManager *IOSAudioManagerBridge::getAudioManager()
{
  return iosAudioManager_;
}

IOSAudioManagerBridge::~IOSAudioManagerBridge()
{
  [iosAudioManager_ cleanup];
}

void IOSAudioManagerBridge::setSessionOptions(std::shared_ptr<SessionOptions> &sessionOptions)
{
  AVAudioSessionMode mode;
  AVAudioSessionCategory category;
  AVAudioSessionCategoryOptions options = 0;

  switch (sessionOptions->iosCategory.value()) {
    case IOSCategory::RECORD:
      category = AVAudioSessionCategoryRecord;
      break;
    case IOSCategory::AMBIENT:
      category = AVAudioSessionCategoryAmbient;
      break;
    case IOSCategory::PLAYBACK:
      category = AVAudioSessionCategoryPlayback;
      break;
    case IOSCategory::MULTI_ROUTE:
      category = AVAudioSessionCategoryMultiRoute;
      break;
    case IOSCategory::SOLO_AMBIENT:
      category = AVAudioSessionCategorySoloAmbient;
      break;
    case IOSCategory::PLAYBACK_AND_RECORD:
      category = AVAudioSessionCategoryPlayAndRecord;
      break;
  }

  switch (sessionOptions->iosMode.value()) {
    case IOSMode::DEFAULT:
      mode = AVAudioSessionModeDefault;
      break;
    case IOSMode::GAME_CHAT:
      mode = AVAudioSessionModeGameChat;
      break;
    case IOSMode::VIDEO_CHAT:
      mode = AVAudioSessionModeVideoChat;
      break;
    case IOSMode::VOICE_CHAT:
      mode = AVAudioSessionModeVoiceChat;
      break;
    case IOSMode::MEASUREMENT:
      mode = AVAudioSessionModeMeasurement;
      break;
    case IOSMode::VOICE_PROMPT:
      mode = AVAudioSessionModeVoicePrompt;
      break;
    case IOSMode::SPOKEN_AUDIO:
      mode = AVAudioSessionModeSpokenAudio;
      break;
    case IOSMode::MOVIE_PLAYBACK:
      mode = AVAudioSessionModeMoviePlayback;
      break;
    case IOSMode::VIDEO_RECORDING:
      mode = AVAudioSessionModeVideoRecording;
      break;
  }

  for (size_t i = 0; i < sessionOptions->iosCategoryOptions.size(); i += 1) {
    AVAudioSessionCategoryOptions option;

    switch (sessionOptions->iosCategoryOptions[i].value()) {
      case IOSCategoryOption::DUCK_OTHERS:
        option = AVAudioSessionCategoryOptionDuckOthers;
        break;
      case IOSCategoryOption::ALLOW_AIR_PLAY:
        option = AVAudioSessionCategoryOptionAllowAirPlay;
        break;
      case IOSCategoryOption::MIX_WITH_OTHERS:
        option = AVAudioSessionCategoryOptionMixWithOthers;
        break;
      case IOSCategoryOption::ALLOW_BLUETOOTH:
        option = AVAudioSessionCategoryOptionAllowBluetooth;
        break;
      case IOSCategoryOption::DEFAULT_TO_SPEAKER:
        option = AVAudioSessionCategoryOptionDefaultToSpeaker;
        break;
      case IOSCategoryOption::ALLOW_BLUETOOTH_A2DP:
        option = AVAudioSessionCategoryOptionAllowBluetoothA2DP;
        break;
      case IOSCategoryOption::OVERRIDE_MUTED_MICROPHONE_INTERRUPTION:
        option = AVAudioSessionCategoryOptionOverrideMutedMicrophoneInterruption;
        break;
      case IOSCategoryOption::INTERRUPT_SPOKEN_AUDIO_AND_MIX_WITH_OTHERS:
        option = AVAudioSessionCategoryOptionInterruptSpokenAudioAndMixWithOthers;
        break;
    }

    options |= option;
  }

  [iosAudioManager_ setSessionOptionsWithCategory:category mode:mode options:options];
}

} // namespace audioapi
