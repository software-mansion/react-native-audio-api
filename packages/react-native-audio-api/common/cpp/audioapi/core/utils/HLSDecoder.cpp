#include <audioapi/core/utils/HLSDecoder.h>

namespace audioapi {

bool HLSDecoder::openFile(int outputSampleRate, const std::string &path) {
  if (avformat_open_input(&fmtCtx_, path, nullptr, nullptr) < 0) {
    return false;
  }
  return avformat_find_stream_info(fmtCtx_, nullptr) >= 0;
}
}; // namespace audioapi