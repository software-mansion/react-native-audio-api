#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>
#include <audioapi/dsp/Convolver.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <atomic>
#include <cstddef>
#include <memory>
#include <vector>

#include <audioapi/utils/ThreadPool.hpp>

static constexpr int GAIN_CALIBRATION =
    -58; // magic number so that processed signal and dry signal have roughly the same volume
static constexpr float MIN_IR_POWER = 0.000125;

namespace audioapi {

struct ConvolverOptions;

/// Channel handling follows the spec's convolution matrix
/// (https://webaudio.github.io/web-audio-api/#Convolution-channel-configurations):
/// the input is negotiated to mono or stereo (`clamped-max`, channelCount <= 2),
/// the output is mono only for a mono input with a mono impulse response and
/// stereo otherwise, and a node without an impulse response emits one channel
/// of silence. The node processes in place in a single buffer that negotiation
/// sizes at the output width.
class ConvolverNode : public AudioNode {
 public:
  explicit ConvolverNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const ConvolverOptions &options);

  /// @brief Installs a prebuilt impulse response and its processing state,
  /// handing the previous ones to the disposer. Everything passed in was
  /// allocated on the JS thread; nothing is allocated here.
  /// @note Audio Thread only
  void setBuffer(
      const std::shared_ptr<AudioBuffer> &buffer,
      std::vector<std::unique_ptr<Convolver>> convolvers,
      const std::shared_ptr<ConvolverThreadPool> &threadPool,
      const std::shared_ptr<DSPAudioBuffer> &internalBuffer,
      const std::shared_ptr<DSPAudioBuffer> &intermediateBuffer,
      float scaleFactor);

  /// @brief Drops the impulse response and its processing state; the node
  /// renders silence until a new one arrives.
  /// @note Audio Thread only
  void clearBuffer();

  /// @brief Appends a convolver built for the installed impulse response (see
  /// `negotiateBufferChannelCount`).
  /// @note Audio Thread only
  void appendConvolver(std::unique_ptr<Convolver> &&convolver);

  /// @brief Records the impulse response negotiation should assume (nullptr
  /// when there is none) and how many convolvers were scheduled for it. Must
  /// be called before the graph renegotiates.
  /// @note JS Thread only
  void setImpulseResponseForNegotiation(
      std::shared_ptr<AudioBuffer> impulseResponse,
      size_t scheduledConvolvers);

  /// @brief Builds a partitioned convolver for one impulse-response channel.
  /// @note JS Thread only (allocates)
  [[nodiscard]] static std::unique_ptr<Convolver> makeConvolver(
      const AudioBuffer &impulseResponse,
      size_t channel);

  float calculateNormalizationScale(const std::shared_ptr<AudioBuffer> &buffer) const;

  /// @brief Output width for a negotiated input width.
  /// @note JS Thread only (negotiation).
  [[nodiscard]] size_t getUpstreamChannelCount(size_t negotiatedChannelCount) const override;

  /// @brief Sizes the in-place buffer at the output width and remembers the
  /// negotiated input width for `mixInputs`.
  ///
  /// This is also where the second convolver of a mono impulse response is
  /// created: each input channel needs its own convolver state, the input
  /// width is only known here, and a mono response is installed with a single
  /// convolver. The first stereo negotiation schedules the second one through
  /// an audio event, once per installed response; it is never removed again,
  /// so it idles if the input becomes mono later. Until that event lands, the
  /// stereo buffer is rendered with the convolvers that exist.
  /// @note JS Thread only (negotiation).
  [[nodiscard]] size_t negotiateBufferChannelCount(size_t negotiatedChannelCount) override;

 protected:
  /// @brief Mixes at the negotiated input width, not the buffer width: a
  /// mono computed input is mixed into a mono scratch first and then copied
  /// to every buffer channel, so multichannel sources fold down with the
  /// spec's mono gains and the mono input drives every IR channel.
  /// @note Audio Thread only
  void mixInputs(const std::vector<const DSPAudioBuffer *> &inputs) override;

  void processNode(int framesToProcess) override;

  /// @brief Tail length equals the impulse-response length in frames. A
  /// freshly loaded IR makes the convolver ring for at least its own length
  /// after the input goes silent.
  /// @note Audio Thread only.
  [[nodiscard]] int computeTailFrames() const override;

 private:
  const float gainCalibrationSampleRate_;
  size_t internalBufferIndex_;
  float scaleFactor_;
  std::shared_ptr<DSPAudioBuffer> intermediateBuffer_;

  // impulse response buffer
  std::shared_ptr<AudioBuffer> buffer_;
  // rendered output not yet handed out, always stereo; the mono case uses channel 0
  std::shared_ptr<DSPAudioBuffer> internalBuffer_;
  // one per impulse-response channel, or one per input channel for a mono response
  std::vector<std::unique_ptr<Convolver>> convolvers_;
  std::shared_ptr<ConvolverThreadPool> threadPool_;

  /// The impulse response negotiation must assume (nullptr = none). Set
  /// ahead of the audio event that installs it, so it can briefly differ
  /// from `buffer_`. JS thread only.
  std::shared_ptr<AudioBuffer> impulseResponseForNegotiation_;
  /// Convolvers scheduled so far for `impulseResponseForNegotiation_`. JS
  /// thread only.
  size_t convolversScheduled_;
  /// Input width negotiation last handed us; narrower than the buffer for a
  /// mono computed input meeting a 2- or 4-channel impulse response.
  std::atomic<size_t> negotiatedInputChannelCount_;
  /// Mixing target when the computed input is mono but the buffer is stereo.
  const std::shared_ptr<DSPAudioBuffer> monoMixBuffer_;

  void releaseImpulseResponse(BaseAudioContext &context);
  [[nodiscard]] size_t outputChannelCountFor(size_t inputChannelCount) const;
  void renderQuantum();
};

} // namespace audioapi
