#include <audioapi/core/AudioContextRegistry.h>

#include <audioapi/core/AudioContext.h>

#include <memory>
#include <utility>
#include <vector>

namespace audioapi {

AudioContextRegistry &AudioContextRegistry::global() {
  static AudioContextRegistry registry;
  return registry;
}

void AudioContextRegistry::add(const std::shared_ptr<AudioContext> &context) {
  std::scoped_lock lock(mutex_);
  std::erase_if(
      contexts_, [](const std::weak_ptr<AudioContext> &entry) { return entry.expired(); });
  contexts_.emplace_back(context);
}

std::vector<std::shared_ptr<AudioContext>> AudioContextRegistry::liveContexts() {
  std::scoped_lock lock(mutex_);
  std::vector<std::shared_ptr<AudioContext>> alive;
  alive.reserve(contexts_.size());

  for (const auto &entry : contexts_) {
    if (auto context = entry.lock()) {
      alive.push_back(std::move(context));
    }
  }

  return alive;
}

} // namespace audioapi
