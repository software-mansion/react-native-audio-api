
#include <IOSAudioManagerBridge.h>

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
  // TODO: delete iosAudioManager?
}

} // namespace audioapi
