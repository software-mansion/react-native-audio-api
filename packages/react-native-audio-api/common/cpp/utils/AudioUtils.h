
namespace audioapi::AudioUtils {
  size_t timeToSampleFrame(double time, int sampleRate) {
    return static_cast<size_t>(time * sampleRate);
  }

  double sampleFrameToTime(int sampleFrame, int sampleRate) {
    return static_cast<double>(sampleFrame) / sampleRate;
  }
} // namespace audioapi::AudioUtils
