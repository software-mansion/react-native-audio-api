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

std::shared_ptr<IOSAudioDestinationNode> IOSAudioContext::getDestination()
{
  return std::make_shared<IOSAudioDestinationNode>(audioContext_.destination);
}

std::shared_ptr<IOSOscillatorNode> IOSAudioContext::createOscillator()
{
  auto oscillator = [audioContext_ createOscillator];
  return std::make_shared<IOSOscillatorNode>(oscillator);
}

std::shared_ptr<IOSGainNode> IOSAudioContext::createGain()
{
  auto gain = [audioContext_ createGain];
  return std::make_shared<IOSGainNode>(gain);
}

std::shared_ptr<IOSStereoPannerNode> IOSAudioContext::createStereoPanner()
{
  auto stereoPanner = [audioContext_ createStereoPanner];
  return std::make_shared<IOSStereoPannerNode>(stereoPanner);
}

std::shared_ptr<IOSBiquadFilterNode> IOSAudioContext::createBiquadFilter()
{
  auto biquadFilter = [audioContext_ createBiquadFilter];
  return std::make_shared<IOSBiquadFilterNode>(biquadFilter);
}

std::shared_ptr<IOSAudioBufferSourceNode> IOSAudioContext::createBufferSource()
{
  auto bufferSource = [audioContext_ createBufferSource];
  return std::make_shared<IOSAudioBufferSourceNode>(bufferSource);
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
