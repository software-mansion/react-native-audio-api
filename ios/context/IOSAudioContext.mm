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

std::shared_ptr<IOSAudioDestinationNode> IOSAudioContext::getDestination() {
    return std::make_shared<IOSAudioDestinationNode>(audioContext_);
}

std::shared_ptr<IOSOscillatorNode> IOSAudioContext::createOscillator() {
    return std::make_shared<IOSOscillatorNode>(audioContext_);
}

std::shared_ptr<IOSGainNode> IOSAudioContext::createGain() {
    return std::make_shared<IOSGainNode>(audioContext_);
}

std::shared_ptr<IOSStereoPannerNode> IOSAudioContext::createStereoPanner() {
    return std::make_shared<IOSStereoPannerNode>(audioContext_);
}

std::shared_ptr<IOSAudioBufferSourceNode> IOSAudioContext::createBufferSource() {
    return std::make_shared<IOSAudioBufferSourceNode>(audioContext_);
}

std::shared_ptr<IOSAudioBuffer> IOSAudioContext::createBuffer() {
    return std::make_shared<IOSAudioBuffer>(audioContext_);
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
