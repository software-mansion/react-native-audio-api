#include <IOSAudioContext.h>

namespace audioapi {

IOSAudioContext::IOSAudioContext()
{
  audioContext_ = [[AudioContext alloc] init];
}

IOSAudioContext::~IOSAudioContext()
{
  [audioContext_ cleanup];
  audioContext_ = nil;
}

void IOSAudioContext::close()
{
  [audioContext_ close];
}

double IOSAudioContext::getCurrentTime()
{
  return [audioContext_ getCurrentTime];
}

std::string IOSAudioContext::getState()
{
  NSString *nsType = [audioContext_ getState];
  return std::string([nsType UTF8String]);
}

double IOSAudioContext::getSampleRate()
{
  return [audioContext_ getSampleRate];
}
} // namespace audioapi
