#include <audioapi/jsi/RuntimeObserver.h>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace audioapi {

static std::unordered_map<jsi::Runtime *, std::unordered_set<RuntimeObserverListener *>> listeners;
// Host objects are destroyed on whichever thread drops their last reference, and runtimes can be
// torn down off the JS thread. Recursive because notifying a listener can destroy other host
// objects, which remove themselves on the same thread.
static std::recursive_mutex listenersMutex;

struct RuntimeObserverObject : public jsi::HostObject {
  jsi::Runtime *rt_;
  explicit RuntimeObserverObject(jsi::Runtime *rt) : rt_(rt) {}
  ~RuntimeObserverObject() override {
    // Notify while holding the lock, so a listener being destroyed on another thread cannot be
    // freed between leaving the set and being notified. Take one listener at a time: a notified
    // listener can remove others from this set.
    std::scoped_lock lock(listenersMutex);
    auto listenersSet = listeners.find(rt_);
    if (listenersSet == listeners.end()) {
      return;
    }
    auto &runtimeListeners = listenersSet->second;
    while (!runtimeListeners.empty()) {
      auto *listener = *runtimeListeners.begin();
      runtimeListeners.erase(runtimeListeners.begin());
      listener->onRuntimeDestroyed(rt_);
    }
    listeners.erase(rt_);
  }
};

void RuntimeObserver::addListener(jsi::Runtime &rt, RuntimeObserverListener *listener) {
  std::scoped_lock lock(listenersMutex);
  auto listenersSet = listeners.find(&rt);
  if (listenersSet == listeners.end()) {
    // We install a global host object in the provided runtime, this way we can
    // use that host object destructor to get notified when the runtime is being
    // terminated. We use a unique name for the object as it gets saved with the
    // runtime's global object.
    rt.global().setProperty(
        rt,
        "__rnaudioapi_runtime_observer",
        jsi::Object::createFromHostObject(rt, std::make_shared<RuntimeObserverObject>(&rt)));
    std::unordered_set<RuntimeObserverListener *> newSet;
    newSet.insert(listener);
    listeners.emplace(&rt, std::move(newSet));
  } else {
    listenersSet->second.insert(listener);
  }
}

void RuntimeObserver::removeListener(jsi::Runtime &rt, RuntimeObserverListener *listener) {
  std::scoped_lock lock(listenersMutex);
  auto listenersSet = listeners.find(&rt);
  if (listenersSet == listeners.end()) {
    // nothing to do here
  } else {
    listenersSet->second.erase(listener);
  }
}

void RuntimeObserver::removeListener(RuntimeObserverListener *listener) {
  std::scoped_lock lock(listenersMutex);
  for (auto &[rt, runtimeListeners] : listeners) {
    runtimeListeners.erase(listener);
  }
}

} // namespace audioapi
