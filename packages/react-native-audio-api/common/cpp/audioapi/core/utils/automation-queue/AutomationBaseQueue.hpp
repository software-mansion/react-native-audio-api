#pragma once

#include "audioapi/core/utils/BaseAutomationEvent.hpp"
#include "audioapi/utils/BoundedPriorityQueue.hpp"

namespace audioapi {

template <typename TEvent>
class AutomationBaseQueue {
 protected:
  struct EventComparator {
    bool operator()(const BaseAutomationEvent &a, const BaseAutomationEvent &b) const {
      return a.getAutomationEventTime() < b.getAutomationEventTime();
    }
  };
  BoundedPriorityQueue<TEvent, 32, EventComparator> eventQueue_;
};

} // namespace audioapi
