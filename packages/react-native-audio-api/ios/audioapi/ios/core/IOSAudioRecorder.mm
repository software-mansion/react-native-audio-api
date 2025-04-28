#import <AVFoundation/AVFoundation.h>

// Redefinition? 🤔
// #define MINIAUDIO_IMPLEMENTATION
// #include <audioapi/libs/miniaudio/miniaudio.h>

#include <audioapi/core/Constants.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/ios/core/IOSAudioRecorder.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

namespace audioapi {

IOSAudioRecorder::IOSAudioRecorder(
    float sampleRate,
    int bufferLength,
    const std::function<void(std::shared_ptr<AudioBus>, int, double)> &onAudioReady)
    : onAudioReady_(onAudioReady), isRunning_(false)
{
  AudioReceiverBlock audioReceiverBlock = ^(const AudioBufferList *inputBuffer, int numFrames, AVAudioTime *when) {
    if (isRunning_.load()) {
      auto bus = std::make_shared<AudioBus>(numFrames, 1, sampleRate);

      // ma_data_converter_config converterConfig = ma_data_converter_config_init(
      //                                                                          ma_format_f32,
      //                                                                          ma_format_f32,
      //                                                                          buffer.audioBufferList->mNumberBuffers,
      //                                                                          numberOfChannels,
      //                                                                          [when sampleRate],
      //                                                                          sampleRate);

      // ma_data_converter dataConverter;
      // ma_data_converter_init(&converterConfig, NULL, &dataConverter);

      // ma_data_converter_process_pcm_frames(&dataConverter, NULL, numFrames, NULL, numFrames);

      auto *inputChannel = (float *)inputBuffer->mBuffers[0].mData;
      auto *outputChannel = bus->getChannel(0)->getData();

      memcpy(outputChannel, inputChannel, numFrames * sizeof(float));
      onAudioReady_(bus, numFrames, [when sampleTime] / [when sampleRate]);

      // ma_data_converter_uninit(&dataConverter, NULL);
    }
  };

  audioRecorder_ = [[NativeAudioRecorder alloc] initWithReceiverBlock:audioReceiverBlock bufferLength:bufferLength];
}

IOSAudioRecorder::~IOSAudioRecorder()
{
  stop();
  [audioRecorder_ cleanup];
}

void IOSAudioRecorder::start()
{
  if (isRunning_.load()) {
    return;
  }

  [audioRecorder_ start];
  isRunning_.store(true);
}

void IOSAudioRecorder::stop()
{
  if (!isRunning_.load()) {
    return;
  }

  isRunning_.store(false);
  [audioRecorder_ stop];
}

} // namespace audioapi
