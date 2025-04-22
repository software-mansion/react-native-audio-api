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
    int numberOfChannels,
    int bufferLength,
    bool enableVoiceProcessing,
    const std::function<void(std::shared_ptr<AudioBus>, int, double)> &onAudioReady)
    : onAudioReady_(onAudioReady)
{
  AudioReceiverBlock audioReceiverBlock = ^(AVAudioPCMBuffer *buffer, int numFrames, AVAudioTime *when) {
    auto bus = std::make_shared<AudioBus>(numFrames, numberOfChannels, sampleRate);

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

    auto *inputChannel = (float *)buffer.mutableAudioBufferList->mBuffers[0].mData;
    auto *outputChannel = bus->getChannel(0)->getData();

    memcpy(outputChannel, inputChannel, numFrames * sizeof(float));
    onAudioReady_(bus, numFrames, [when sampleTime] / [when sampleRate]);

    // ma_data_converter_uninit(&dataConverter, NULL);
  };

  audioRecorder_ = [[CAudioRecorder alloc] initWithReceiverBlock:audioReceiverBlock
                                                    bufferLength:bufferLength
                                           enableVoiceProcessing:enableVoiceProcessing];
}

IOSAudioRecorder::~IOSAudioRecorder()
{
  [audioRecorder_ cleanup];
}

void IOSAudioRecorder::start()
{
  [audioRecorder_ start];
}

void IOSAudioRecorder::stop()
{
  [audioRecorder_ stop];
}

} // namespace audioapi
