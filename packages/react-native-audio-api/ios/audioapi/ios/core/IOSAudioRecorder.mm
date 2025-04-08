#import <AVFoundation/AVFoundation.h>

#include <audioapi/ios/core/IOSAudioRecorder.h>

namespace audioapi {

IOSAudioRecorder::IOSAudioRecorder()
{
  audioRecorder_ = [[CAudioRecorder alloc] init];
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
