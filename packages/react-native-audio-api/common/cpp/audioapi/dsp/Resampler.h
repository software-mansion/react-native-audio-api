#pragma once

#include <audioapi/core/utils/Constants.h>
#include <audioapi/utils/AudioArray.h>
#include <memory>
#include <vector>

namespace audioapi {

class Resampler {
 public:
  /// Constructor
  /// @param maxBlockSize Maximum block size that will be processed
  /// @param kernelSize Size of the resampling kernel
  /// @note maxBlockSize >= kernelSize
  Resampler(int maxBlockSize, int kernelSize);
  virtual ~Resampler() = default;

  virtual int process(
      const std::shared_ptr<AudioArray> &input,
      const std::shared_ptr<AudioArray> &output,
      int framesToProcess) = 0;
  void reset();

 protected:
  [[nodiscard]] float computeBlackmanWindow(double x) const;
  float computeConvolution(const float *stateStart, const float *kernelStart) const;
  virtual void initializeKernel() = 0;

  int kernelSize_;

  std::shared_ptr<AudioArray> kernel_;
  // [ HISTORY | NEW DATA ]
  std::shared_ptr<AudioArray> stateBuffer_;
};

class UpSampler : public Resampler {
 public:
  UpSampler(int maxBlockSize, int kernelSize);

  // N -> 2N
  int process(
      const std::shared_ptr<AudioArray> &input,
      const std::shared_ptr<AudioArray> &output,
      int framesToProcess) override;

 protected:
  void initializeKernel() final;
};

class DownSampler : public Resampler {
 public:
  DownSampler(int maxBlockSize, int kernelSize);

  // N -> N / 2
  int process(
      const std::shared_ptr<AudioArray> &input,
      const std::shared_ptr<AudioArray> &output,
      int framesToProcess) override;

 protected:
  void initializeKernel() final;
};

} // namespace audioapi
