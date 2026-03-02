#pragma once

#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/effects/BiquadFilterNode.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <memory>

static constexpr int sampleRate = 44100;
static constexpr float nyquistFrequency = sampleRate / 2.0f;
static constexpr float tolerance = 0.0001f;

namespace audioapi {
class BiquadFilterTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(
        2, 5 * sampleRate, sampleRate, eventRegistry, RuntimeRegistry{});
  }

  void expectCoefficientsNear(const BiquadFilterNode &node, const BiquadCoefficients &expected);
  void testLowpass(float frequency, float Q);
  void testHighpass(float frequency, float Q);
  void testBandpass(float frequency, float Q);
  void testNotch(float frequency, float Q);
  void testAllpass(float frequency, float Q);
  void testPeaking(float frequency, float Q, float gain);
  void testLowshelf(float frequency, float gain);
  void testHighshelf(float frequency, float gain);
};

class BiquadFilterQTestLowpassHighpass : public BiquadFilterTest,
                                         public ::testing::WithParamInterface<float> {};
class BiquadFilterQTestRestTypes : public BiquadFilterTest,
                                   public ::testing::WithParamInterface<float> {};
class BiquadFilterFrequencyTest : public BiquadFilterTest,
                                  public ::testing::WithParamInterface<float> {};
class BiquadFilterGainTest : public BiquadFilterTest,
                             public ::testing::WithParamInterface<float> {};
} // namespace audioapi
