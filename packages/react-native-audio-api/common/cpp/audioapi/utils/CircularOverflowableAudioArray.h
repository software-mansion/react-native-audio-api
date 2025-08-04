
#pragma once

#include <audioapi/utils/AudioArray.h>
#include <stdexcept>
#include <atomic>
#include <mutex>

namespace audioapi {

/// @brief CircularOverflowableAudioArray is a circular audio array that allows for overflow.
/// It provides a way to push data safely from one thread while reading from another.
/// It is designed to handle cases where the write index may be updated while the read index is being read.
/// It has read precedence, meaning that read locks are acquired before write locks.
/// @note read can fail when there are a lot of writes and buffer is small.
class CircularOverflowableAudioArray : public AudioArray {
 public:
  explicit CircularOverflowableAudioArray(size_t size);
  CircularOverflowableAudioArray(const CircularOverflowableAudioArray &other) = delete;
  ~CircularOverflowableAudioArray() = default;

  /// @brief Writes data to the circular buffer.
  /// @note Might wait for read operation to finish if it is in progress. It ignores writes that exceed the buffer size.
  /// @param data Pointer to the input buffer.
  /// @param size Number of frames to write.
  void write(const float *data, size_t size);

  /// @brief Reads data from the circular buffer.
  /// @param output Pointer to the output buffer.
  /// @param size Number of frames to read.
  /// @return The number of frames actually read.
  size_t read(float *output, size_t size) const;

 private:
  std::atomic<size_t> vWriteIndex_ = { 0 };
  mutable size_t vReadIndex_ = 0; // it is only used after acquiring readLock_
  mutable std::mutex readLock_;

  [[nodiscard]] size_t getAvailableSpace() const;
};

} // namespace audioapi
