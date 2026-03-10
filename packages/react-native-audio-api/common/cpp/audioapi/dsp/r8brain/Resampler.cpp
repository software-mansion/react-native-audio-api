#include <audioapi/dsp/r8brain/Resampler.h>

namespace r8b {

BaseResampler::BaseResampler(double srcRate, double dstRate, int numChannels, int maxInLen) {
  resamplers_.reserve(static_cast<size_t>(numChannels));
  inputBuffers_.reserve(static_cast<size_t>(numChannels));
  for (int i = 0; i < numChannels; ++i) {
    resamplers_.emplace_back(std::make_unique<CDSPResampler24>(srcRate, dstRate, maxInLen));
    inputBuffers_.emplace_back(static_cast<size_t>(maxInLen));
  }
}

int BaseResampler::process(const std::vector<float *> &input, int l, std::vector<float *> &output) {
  int outLen = 0;
  const size_t numChannels = resamplers_.size();

  for (size_t i = 0; i < numChannels; ++i) {
    const float *__restrict inData = input[i];
    double *__restrict bufData = inputBuffers_[i].data();

    for (int j = 0; j < l; ++j) {
      bufData[j] = static_cast<double>(inData[j]);
    }

    double *outPtr = nullptr;
    const int currentOutLen = resamplers_[i]->process(bufData, l, outPtr);
    outLen = currentOutLen;

    if (currentOutLen > 0 && outPtr != nullptr) {
      const double *__restrict resampledData = outPtr;
      float *__restrict outData = output[i];

      for (int j = 0; j < currentOutLen; ++j) {
        outData[j] = static_cast<float>(resampledData[j]);
      }
    }
  }

  return outLen;
}

int BaseResampler::getMaxOutLen() const {
  if (resamplers_.empty()) {
    return 0;
  }
  return resamplers_[0]->getMaxOutLen(0);
}

MultiChannelResampler::MultiChannelResampler(
    double srcRate,
    double dstRate,
    int numChannels,
    int maxInLen)
    : BaseResampler(srcRate, dstRate, numChannels, maxInLen) {}

int MultiChannelResampler::process(
    const audioapi::AudioBuffer &input,
    int l,
    audioapi::AudioBuffer &output) {
  const size_t numChannels = input.getNumberOfChannels();
  std::vector<float *> inputPtrs(numChannels);
  std::vector<float *> outputPtrs(numChannels);
  for (size_t i = 0; i < numChannels; ++i) {
    inputPtrs[i] = input.getChannel(i)->begin();
    outputPtrs[i] = output.getChannel(i)->begin();
  }
  return BaseResampler::process(inputPtrs, l, outputPtrs);
}

SingleChannelResampler::SingleChannelResampler(double srcRate, double dstRate, int maxInLen)
    : BaseResampler(srcRate, dstRate, 1, maxInLen) {}

int SingleChannelResampler::process(
    const audioapi::AudioArray &input,
    int l,
    audioapi::AudioArray &output) {
  std::vector<float *> inputPtrs(1);
  std::vector<float *> outputPtrs(1);
  inputPtrs[0] = const_cast<float *>(input.begin());
  outputPtrs[0] = output.begin();
  return BaseResampler::process(inputPtrs, l, outputPtrs);
}

} // namespace r8b
