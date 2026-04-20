#pragma once

#include <audioapi/core/utils/Constants.h>
#include <audioapi/utils/BoundedPriorityQueue.hpp>
#include <audioapi/utils/Macros.h>
#include <utility>

namespace audioapi {

template <typename TEvent>
concept ParamEventConcept = requires(TEvent event) {
  { event.getAutomationTime() } -> std::convertible_to<double>;
};
template <ParamEventConcept TEvent>
class ParamQueueBase {
 public:
  ParamQueueBase() = default;
  DELETE_COPY_AND_MOVE(ParamQueueBase);
  virtual ~ParamQueueBase() = default;

  /// @brief Cancel scheduled parameter changes at or after the given time.
  /// @param cancelTime The time at which to cancel scheduled changes.
  void cancelScheduledValues(double cancelTime) {
    eventQueue_.erase(eventQueue_.lowerBound(cancelTime), eventQueue_.end());
  }

  virtual bool push(TEvent &&event) {
    return eventQueue_.push(std::move(event));
  }

 protected:
  struct EventComparator {
    using is_transparent = void;
    bool operator()(const TEvent &a, const TEvent &b) const {
      return a.getAutomationTime() < b.getAutomationTime();
    }
    bool operator()(const TEvent &a, double time) const {
      return a.getAutomationTime() < time;
    }
    bool operator()(double time, const TEvent &b) const {
      return time < b.getAutomationTime();
    }
  };

  BoundedPriorityQueue<TEvent, AUDIO_PARAM_MAX_QUEUED_EVENTS, EventComparator> eventQueue_;
};

} // namespace audioapi
