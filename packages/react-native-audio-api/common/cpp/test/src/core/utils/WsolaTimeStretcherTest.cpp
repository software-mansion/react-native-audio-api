#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/WsolaTimeStretcher.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <gtest/gtest.h>

#include <cmath>

using namespace audioapi;

namespace {

float sumAbs(const DSPAudioBuffer &buffer) {
  float result = 0.0f;
  for (size_t channel = 0; channel < buffer.getNumberOfChannels(); ++channel) {
    const float *data = buffer.getChannel(channel)->begin();
    for (size_t frame = 0; frame < buffer.getSize(); ++frame) {
      result += std::abs(data[frame]);
    }
  }
  return result;
}

} // namespace

TEST(WsolaTimeStretcherTest, ProducesOutputAfterInputIsBuffered) {
  static constexpr float sampleRate = 48000.0f;
  static constexpr size_t inputFrames = 256;
  static constexpr size_t outputFrames = RENDER_QUANTUM_SIZE;

  WsolaTimeStretcher stretcher;
  stretcher.configure(1, sampleRate);

  DSPAudioBuffer input(inputFrames, 1, sampleRate);
  DSPAudioBuffer output(outputFrames, 1, sampleRate);

  float totalOutput = 0.0f;
  size_t absoluteFrame = 0;
  for (size_t chunk = 0; chunk < 20; ++chunk) {
    float *inputData = input.getChannel(0)->begin();
    for (size_t frame = 0; frame < inputFrames; ++frame) {
      inputData[frame] =
          std::sin(2.0f * PI * 440.0f * static_cast<float>(absoluteFrame++) / sampleRate);
    }

    stretcher.process(input, inputFrames, output, outputFrames, 1.5f);
    totalOutput += sumAbs(output);
  }

  EXPECT_GT(totalOutput, 0.0f);
}

TEST(WsolaTimeStretcherTest, ResetClearsBufferedOutput) {
  static constexpr float sampleRate = 48000.0f;
  static constexpr size_t outputFrames = RENDER_QUANTUM_SIZE;

  WsolaTimeStretcher stretcher;
  stretcher.configure(1, sampleRate);
  stretcher.reset();

  DSPAudioBuffer emptyInput(0, 1, sampleRate);
  DSPAudioBuffer output(outputFrames, 1, sampleRate);

  stretcher.process(emptyInput, 0, output, outputFrames, 1.5f);

  EXPECT_FLOAT_EQ(sumAbs(output), 0.0f);
}
