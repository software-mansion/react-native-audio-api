#include <audioapi/core/utils/AudioEventScheduler.h>
#include <audioapi/core/BaseAudioContext.h>

#include <memory>
#include <utility>
#include <functional>

namespace audioapi {

AudioEventScheduler::AudioEventScheduler(const std::shared_ptr<BaseAudioContext>& context,
                                         size_t capacity): context_(context), eventScheduler_(capacity) {}

bool AudioEventScheduler::scheduleEvent(std::function<void(BaseAudioContext &)> &&event) noexcept {
    if (context_->isRunning()) {
        return eventScheduler_.scheduleEvent(std::move(event));
    } else {
      event(*context_);
      return true;
    }
}

void AudioEventScheduler::processAllEvents() {
  eventScheduler_.processAllEvents(*context_);
}

} // namespace audioapi
