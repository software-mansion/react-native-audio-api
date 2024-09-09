#include <IOSAudioParam.h>

namespace audioapi {

IOSAudioParam::IOSAudioParam(AudioParam *audioParam) {
    audioParam_ = audioParam;
}

IOSAudioParam::~IOSAudioParam()
{
  [audioParam_ cleanup];
  audioParam_ = nil;
}

void IOSAudioParam::setValue(float value)
{
  [audioParam_ setValue:value];
}

float IOSAudioParam::getValue()
{
  return [audioParam_ getValue];
}

float IOSAudioParam::getMinValue()
{
  return [audioParam_ getMinValue];
}

float IOSAudioParam::getMaxValue()
{
  return [audioParam_ getMaxValue];
}

float IOSAudioParam::getDefaultValue()
{
  return [audioParam_ getDefaultValue];
}

void IOSAudioParam::setValueAtTime(float value, double time)
{
  [audioParam_ setValueAtTime:value time:time];
}

void IOSAudioParam::linearRampToValueAtTime(float value, double time)
{
  [audioParam_ linearRampToValueAtTime:value time:time];
}

void IOSAudioParam::exponentialRampToValueAtTime(float value, double time)
{
  [audioParam_ exponentialRampToValueAtTime:value time:time];
}

} // namespace audioapi
