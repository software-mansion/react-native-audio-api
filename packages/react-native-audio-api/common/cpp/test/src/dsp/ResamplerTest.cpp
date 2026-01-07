#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/dsp/Resampler.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <audioapi/core/utils/Constants.h>
#include <gtest/gtest.h>
#include <memory>

using namespace audioapi;

class ResamplerTest : public ::testing::Test {
  protected:
    static constexpr int KERNEL_SIZE = RENDER_QUANTUM_SIZE;
};

class TestableUpSampler : public UpSampler {
 public:
  explicit TestableUpSampler(int maxBlockSize, int kernelSize)
      : UpSampler(maxBlockSize, kernelSize) {}

  std::shared_ptr<AudioArray> getKernel() {
    return kernel_;
  }
};

class TestableDownSampler : public DownSampler {
 public:
  explicit TestableDownSampler(int maxBlockSize, int kernelSize)
      : DownSampler(maxBlockSize, kernelSize) {}

  std::shared_ptr<AudioArray> getKernel() {
    return kernel_;
  }
};

TEST_F(ResamplerTest, UpSamplerCanBeCreated) {
  auto upSampler = std::make_unique<UpSampler>(RENDER_QUANTUM_SIZE, RENDER_QUANTUM_SIZE);
  ASSERT_NE(upSampler, nullptr);
}

TEST_F(ResamplerTest, DownSamplerCanBeCreated) {
  auto downSampler = std::make_unique<DownSampler>(RENDER_QUANTUM_SIZE * 2, RENDER_QUANTUM_SIZE * 2);
  ASSERT_NE(downSampler, nullptr);
}

TEST_F(ResamplerTest, UpSamplerKernelSymmetry) {
  auto upSampler = std::make_unique<TestableUpSampler>(RENDER_QUANTUM_SIZE, RENDER_QUANTUM_SIZE);
  auto kernel = upSampler->getKernel();

  // check for symmetry around the center point
  for (size_t i = 0; i < kernel->getSize() / 2; ++i) {
    EXPECT_NEAR((*kernel)[i], (*kernel)[kernel->getSize() - 1 - i], 1e-6);
  }
}

TEST_F(ResamplerTest, DownSamplerKernelSymmetry) {
  auto downSampler =
      std::make_unique<TestableDownSampler>(RENDER_QUANTUM_SIZE * 2, RENDER_QUANTUM_SIZE * 2);
  auto kernel = downSampler->getKernel();

  // check for symmetry around the center point
  // as the kernel size is even, we compare pairs around the center -> kernel[size/2 - 1]
  // last value is skipped
  for (size_t i = 0; i < kernel->getSize() / 2; ++i) {
    EXPECT_NEAR((*kernel)[i], (*kernel)[kernel->getSize() - 2 - i], 1e-6);
  }

  EXPECT_FLOAT_EQ((*kernel)[kernel->getSize() / 2 - 1], 0.5f);
}

TEST_F(ResamplerTest, UpSamplerKernelSum) {
  auto upSampler = std::make_unique<TestableUpSampler>(RENDER_QUANTUM_SIZE, RENDER_QUANTUM_SIZE);
  auto kernel = upSampler->getKernel();

  float sum = 0.0f;
  for (size_t i = 0; i < kernel->getSize(); ++i) {
    sum += (*kernel)[i];
  }

  EXPECT_NEAR(sum, 1.0f, 1e-6);
}

TEST_F(ResamplerTest, DownSamplerKernelSum) {
  auto downSampler =
      std::make_unique<TestableDownSampler>(RENDER_QUANTUM_SIZE * 2, RENDER_QUANTUM_SIZE * 2);
  auto kernel = downSampler->getKernel();

  float sum = 0.0f;
  for (size_t i = 0; i < kernel->getSize(); ++i) {
    sum += (*kernel)[i];
  }

  EXPECT_NEAR(sum, 1.0f, 1e-6);
}

TEST_F(ResamplerTest, UpDownSamplingProcess) {
  auto upSampler = std::make_unique<TestableUpSampler>(RENDER_QUANTUM_SIZE, RENDER_QUANTUM_SIZE);
  auto downSampler =
      std::make_unique<TestableDownSampler>(RENDER_QUANTUM_SIZE * 2, RENDER_QUANTUM_SIZE * 2);

  auto inputArray = std::make_shared<AudioArray>(4);
  (*inputArray)[0] = 1.0f;
  (*inputArray)[1] = 0.0f;
  (*inputArray)[2] = -1.0f;
  (*inputArray)[3] = 1.0f;

  auto outputArray = std::make_shared<AudioArray>(8);

  int upSamplerOutputFrames;
  int downSamplerOutputFrames;

  EXPECT_NO_THROW(upSamplerOutputFrames = upSampler->process(inputArray, outputArray, 4));
  EXPECT_NO_THROW(downSamplerOutputFrames = downSampler->process(outputArray, inputArray, 8));

  EXPECT_EQ(upSamplerOutputFrames, 8);
  EXPECT_EQ(downSamplerOutputFrames, 4);
}
