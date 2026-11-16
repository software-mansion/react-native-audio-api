#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace audioapi {

class AudioRecorder;

/// @brief What an open input reports to the recorder that opened it, from the input's own threads.
// Events from an input that has been closed since, or whose recorder is gone, are dropped,
// so a late platform notification cannot reach another session.
class RecorderInputEvents {
 public:
  /// The platform reconfigured the open input on its own (an iOS route change).
  void formatChanged() const;
  /// The capture stream died and has to be reopened (an Android disconnect).
  void lost() const;
  /// The platform gave up recovering the input; @p reason says why.
  void failed(const std::string &reason) const;

 private:
  friend class AudioRecorder;

  RecorderInputEvents(std::weak_ptr<AudioRecorder> recorder, uint64_t openedGeneration)
      : recorder_(std::move(recorder)), openedGeneration_(openedGeneration) {}

  std::weak_ptr<AudioRecorder> recorder_;
  /// AudioRecorder's input generation when the input was opened.
  uint64_t openedGeneration_;
};

} // namespace audioapi
