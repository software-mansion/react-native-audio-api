#include <audioapi/dsp/r8brain/Resampler.h>

namespace r8b {

MultiChannelResampler::MultiChannelResampler(
    double srcRate,
    double dstRate,
    int numChannels,
    int maxInLen) {
  for (int i = 0; i < numChannels; ++i) {
    resamplers_.emplace_back(std::make_unique<CDSPResampler24>(srcRate, dstRate, maxInLen));
    inputBuffers_.emplace_back(maxInLen);
    outputBuffers_.emplace_back();
  }
}

int MultiChannelResampler::process(
    const audioapi::AudioBuffer &input,
    int l,
    audioapi::AudioBuffer &output) {
  std::vector<float *> inputPtrs(input.getNumberOfChannels());
  std::vector<float *> outputPtrs(input.getNumberOfChannels());
  for (int i = 0; i < input.getNumberOfChannels(); ++i) {
    inputPtrs[i] = input.getChannel(i)->begin();
    outputPtrs[i] = output.getChannel(i)->begin();
  }
  return process(inputPtrs, l, outputPtrs);
}

int MultiChannelResampler::process(
    const std::vector<float *> &input,
    int l,
    std::vector<float *> &output) {
  int outLen = 0;
  const size_t numChannels = resamplers_.size();

  for (size_t i = 0; i < numChannels; ++i) {
    auto &buf = inputBuffers_[i];

    if (buf.size() < static_cast<size_t>(l)) {
      buf.resize(l);
    }

    // Use restricted to guarantee SIMD auto-vectorization
    const float *__restrict inData = input[i];
    double *__restrict bufData = buf.data();

    for (int j = 0; j < l; ++j) {
      bufData[j] = static_cast<double>(inData[j]);
    }

    double *outPtr = nullptr;
    const int currentOutLen = resamplers_[i]->process(bufData, l, outPtr);
    outLen = currentOutLen;

    if (currentOutLen > 0 && outPtr != nullptr) {
      auto &obuf = outputBuffers_[i];
      if (obuf.size() < static_cast<size_t>(currentOutLen)) {
        obuf.resize(currentOutLen);
      }

      // Use restricted to guarantee SIMD auto-vectorization
      const double *__restrict resampledData = outPtr;
      float *__restrict obufData = obuf.data();

      for (int j = 0; j < currentOutLen; ++j) {
        obufData[j] = static_cast<float>(resampledData[j]);
      }

      output[i] = obufData;
    }
  }

  return outLen;
}
} // namespace r8b
