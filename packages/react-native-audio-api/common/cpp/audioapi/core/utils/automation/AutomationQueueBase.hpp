#pragma once

#include "audioapi/core/utils/automation/AutomationEvent.hpp"
#include "audioapi/utils/BoundedPriorityQueue.hpp"

namespace audioapi {

template <typename TEvent>
class AutomationQueueBase {
 protected:
  struct EventComparator {
    bool operator()(const AutomationEvent &a, const AutomationEvent &b) const {
      return a.getAutomationEventTime() < b.getAutomationEventTime();
    }
  };

  BoundedPriorityQueue<TEvent, 32, EventComparator> eventQueue_;
};

} // namespace audioapi
