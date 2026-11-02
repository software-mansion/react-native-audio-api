#pragma once

#include <audioapi/jsi/RuntimeObserver.h>

#include <jsi/jsi.h>
#include <mutex>
#include <unordered_map>
#include <utility>

namespace audioapi {

using namespace facebook;

/**
 * Provides a way to keep data specific to a jsi::Runtime instance that gets
 * cleaned up when that runtime is destroyed. This is necessary because JSI does
 * not allow for its associated objects to be retained past the runtime
 * lifetime. If an object (e.g. jsi::Values or jsi::Function instances) is kept
 * after the runtime is torn down, its destructor (once it is destroyed
 * eventually) will result in a crash (JSI objects keep a pointer to memory
 * managed by the runtime, accessing that portion of the memory after runtime is
 * deleted is the root cause of that crash).
 */
template <typename T>
class RuntimeInstanceCache : public RuntimeObserverListener {
 public:
  void onRuntimeDestroyed(jsi::Runtime *rt) override {
    std::scoped_lock lock(runtimeCachesMutex_);
    runtimeCaches_.erase(rt);
  }

  ~RuntimeInstanceCache() override {
    // Unregister under the observer's lock rather than walking `runtimeCaches_`, which a runtime
    // being torn down on another thread may be erasing from at the same time.
    RuntimeObserver::removeListener(this);
  }

  T &get(jsi::Runtime &rt) {
    {
      std::scoped_lock lock(runtimeCachesMutex_);
      if (auto cache = runtimeCaches_.find(&rt); cache != runtimeCaches_.end()) {
        return cache->second;
      }
    }
    // Registered without holding `runtimeCachesMutex_`: teardown takes the observer's lock and
    // then this one, so taking them in the opposite order here could deadlock.
    RuntimeObserver::addListener(rt, this);

    std::scoped_lock lock(runtimeCachesMutex_);
    // first time this runtime has seen this cache, create it and return it.
    return runtimeCaches_.emplace(&rt, T{}).first->second;
  }

 private:
  // Guards the map structure: `get` runs on each runtime's JS thread while `onRuntimeDestroyed`
  // runs on whichever thread tears another runtime down.
  std::mutex runtimeCachesMutex_;
  std::unordered_map<jsi::Runtime *, T> runtimeCaches_;
};

} // namespace audioapi
