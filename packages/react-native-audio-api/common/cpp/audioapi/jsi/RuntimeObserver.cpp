#include <audioapi/jsi/RuntimeObserver.h>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace audioapi {

static std::unordered_map<jsi::Runtime *, std::unordered_set<RuntimeObserverListener *>> listeners;
static std::mutex listenersMutex;

struct RuntimeObserverObject : public jsi::HostObject {
  jsi::Runtime *rt_;
  explicit RuntimeObserverObject(jsi::Runtime *rt) : rt_(rt) {}
  ~RuntimeObserverObject() override {
    std::unordered_set<RuntimeObserverListener *> toNotify;
    {
      std::scoped_lock lock(listenersMutex);
      auto listenersSet = listeners.find(rt_);
      if (listenersSet != listeners.end()) {
        toNotify = std::move(listenersSet->second);
        listeners.erase(listenersSet);
      }
    }
    // Notify outside the lock — listener callbacks may re-enter add/remove.
    for (auto *listener : toNotify) {
      listener->onRuntimeDestroyed(rt_);
    }
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

} // namespace audioapi
