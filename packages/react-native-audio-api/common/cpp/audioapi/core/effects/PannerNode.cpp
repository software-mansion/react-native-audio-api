#include <audioapi/core/AudioListener.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/PannerNode.h>
#include <audioapi/core/effects/PannerSpatialization.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>

#include <cmath>
#include <memory>

namespace audioapi {

namespace {

using panner::DEG_180;
using panner::DEG_90;
using panner::Vec3;

std::shared_ptr<AudioParam> createPannerParam(
    float defaultValue,
    const std::shared_ptr<BaseAudioContext> &context) {
  return std::make_shared<AudioParam>(
      defaultValue, MOST_NEGATIVE_SINGLE_FLOAT, MOST_POSITIVE_SINGLE_FLOAT, context);
}

} // namespace

PannerNode::PannerNode(
    const std::shared_ptr<BaseAudioContext> &context,
    AudioListener *listener,
    const PannerOptions &options)
    : AudioNode(context, options),
      listener_(listener),
      positionXParam_(createPannerParam(options.positionX, context)),
      positionYParam_(createPannerParam(options.positionY, context)),
      positionZParam_(createPannerParam(options.positionZ, context)),
      orientationXParam_(createPannerParam(options.orientationX, context)),
      orientationYParam_(createPannerParam(options.orientationY, context)),
      orientationZParam_(createPannerParam(options.orientationZ, context)),
      panningModel_(options.panningModel),
      distanceModel_(options.distanceModel),
      refDistance_(options.refDistance),
      maxDistance_(options.maxDistance),
      rolloffFactor_(options.rolloffFactor),
      coneInnerAngle_(options.coneInnerAngle),
      coneOuterAngle_(options.coneOuterAngle),
      coneOuterGain_(options.coneOuterGain),
      outputBuffer_(
          std::make_shared<DSPAudioBuffer>(
              RENDER_QUANTUM_SIZE,
              channelCount_,
              context->getSampleRate())) {}

void PannerNode::processNode(int framesToProcess) {
  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr || audioBuffer_ == nullptr || listener_ == nullptr) {
    outputBuffer_->zero();
    return;
  }

  const double time = context->getCurrentTime();
  const bool monoInput = audioBuffer_->getNumberOfChannels() == 1;

  listener_->processForQuantum(framesToProcess, time, context->getCurrentSampleFrame());

  const auto posX =
      positionXParam_->processARateParam(framesToProcess, time)->getChannel(0)->span();
  const auto posY =
      positionYParam_->processARateParam(framesToProcess, time)->getChannel(0)->span();
  const auto posZ =
      positionZParam_->processARateParam(framesToProcess, time)->getChannel(0)->span();
  const auto orientX =
      orientationXParam_->processARateParam(framesToProcess, time)->getChannel(0)->span();
  const auto orientY =
      orientationYParam_->processARateParam(framesToProcess, time)->getChannel(0)->span();
  const auto orientZ =
      orientationZParam_->processARateParam(framesToProcess, time)->getChannel(0)->span();

  const auto listenerPosX = listener_->positionXValues();
  const auto listenerPosY = listener_->positionYValues();
  const auto listenerPosZ = listener_->positionZValues();
  const auto listenerForwardX = listener_->forwardXValues();
  const auto listenerForwardY = listener_->forwardYValues();
  const auto listenerForwardZ = listener_->forwardZValues();
  const auto listenerUpX = listener_->upXValues();
  const auto listenerUpY = listener_->upYValues();
  const auto listenerUpZ = listener_->upZValues();

  auto outputLeft = outputBuffer_->getChannelByType(AudioBuffer::ChannelLeft)->span();
  auto outputRight = outputBuffer_->getChannelByType(AudioBuffer::ChannelRight)->span();

  auto inputLeftSpan = monoInput ? audioBuffer_->getChannelByType(AudioBuffer::ChannelMono)->span()
                                 : audioBuffer_->getChannelByType(AudioBuffer::ChannelLeft)->span();
  auto inputRightSpan =
      monoInput ? inputLeftSpan : audioBuffer_->getChannelByType(AudioBuffer::ChannelRight)->span();

  // Equal-power is the only supported panning model.
  (void)panningModel_;

  for (int i = 0; i < framesToProcess; ++i) {
    const auto idx = static_cast<size_t>(i);
    const Vec3 sourcePosition{.x = posX[idx], .y = posY[idx], .z = posZ[idx]};
    const Vec3 sourceOrientation{.x = orientX[idx], .y = orientY[idx], .z = orientZ[idx]};
    const Vec3 listenerPosition{
        .x = listenerPosX[idx], .y = listenerPosY[idx], .z = listenerPosZ[idx]};
    const Vec3 listenerForward{
        .x = listenerForwardX[idx], .y = listenerForwardY[idx], .z = listenerForwardZ[idx]};
    const Vec3 listenerUp{.x = listenerUpX[idx], .y = listenerUpY[idx], .z = listenerUpZ[idx]};

    double azimuth =
        panner::computeAzimuth(sourcePosition, listenerPosition, listenerForward, listenerUp);

    azimuth = panner::clampAzimuth(azimuth);
    azimuth = panner::wrapAzimuth(azimuth);

    double x = 0.0;
    if (monoInput) {
      x = (azimuth + DEG_90) / DEG_180;
    } else if (azimuth <= 0.0) {
      x = (azimuth + DEG_90) / DEG_90;
    } else {
      x = azimuth / DEG_90;
    }

    const double gainL = std::cos(x * panner::PI / 2.0);
    const double gainR = std::sin(x * panner::PI / 2.0);

    const double inputL = inputLeftSpan[idx];
    const double inputR = inputRightSpan[idx];

    double panLeft = 0.0;
    double panRight = 0.0;

    if (monoInput) {
      panLeft = inputL * gainL;
      panRight = inputL * gainR;
    } else if (azimuth <= 0.0) {
      panLeft = inputL + inputR * gainL;
      panRight = inputR * gainR;
    } else {
      panLeft = inputL * gainL;
      panRight = inputR + inputL * gainR;
    }

    const double distance = panner::computeDistance(sourcePosition, listenerPosition);
    const double distanceGain = panner::computeDistanceGain(
        distanceModel_, distance, refDistance_, maxDistance_, rolloffFactor_);
    const double coneGain = panner::computeConeGain(
        sourcePosition,
        listenerPosition,
        sourceOrientation,
        coneInnerAngle_,
        coneOuterAngle_,
        coneOuterGain_);
    const double totalGain = coneGain * distanceGain;

    outputLeft[idx] = static_cast<float>(totalGain * panLeft);
    outputRight[idx] = static_cast<float>(totalGain * panRight);
  }
}

} // namespace audioapi
