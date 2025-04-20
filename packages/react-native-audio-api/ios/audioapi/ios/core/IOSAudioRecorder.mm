#import <AVFoundation/AVFoundation.h>

#include <audioapi/core/Constants.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/ios/core/IOSAudioRecorder.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

namespace audioapi {

IOSAudioRecorder::IOSAudioRecorder(const std::function<void(std::shared_ptr<AudioBus>, int, double)> &onAudioReady)
    : onAudioReady_(onAudioReady)
{
  AudioReceiverBlock audioReceiverBlock = ^(AVAudioPCMBuffer *buffer, int numFrames, AVAudioTime *when) {
    auto bus = std::make_shared<AudioBus>(numFrames, 1, [when sampleRate]);

    auto *inputChannel = (float *)buffer.mutableAudioBufferList->mBuffers[0].mData;
    auto *outputChannel = bus->getChannel(0)->getData();

    memcpy(outputChannel, inputChannel, numFrames * sizeof(float));
    onAudioReady_(bus, numFrames, [when sampleTime] / [when sampleRate]);
  };

  audioRecorder_ = [[CAudioRecorder alloc] initWithReceiverBlock:audioReceiverBlock];
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
