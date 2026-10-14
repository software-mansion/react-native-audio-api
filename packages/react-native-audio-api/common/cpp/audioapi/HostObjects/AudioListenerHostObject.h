#pragma once

#include <audioapi/core/utils/graph/Graph.h>
#include <audioapi/core/utils/graph/HostNode.h>
#include <audioapi/jsi/HostObject.h>

#include <jsi/jsi.h>
#include <memory>

namespace audioapi {
using namespace facebook;

class AudioListener;
class AudioParamHostObject;
class BaseAudioContext;

/// @brief JSI bridge for the AudioListener interface.
///
/// AudioListener is not an AudioNode. The HostObject owns the listener state
/// and a lightweight graph anchor so AudioParam bridge nodes can attach for
/// topological ordering when params are connected to other nodes.
///
/// Deprecated `setPosition` / `setOrientation` are intentionally omitted.
class AudioListenerHostObject : public HostObject, public utils::graph::HostNode {
 public:
  explicit AudioListenerHostObject(const std::shared_ptr<BaseAudioContext> &context);
  ~AudioListenerHostObject() override;

  JSI_PROPERTY_GETTER_DECL(positionX);
  JSI_PROPERTY_GETTER_DECL(positionY);
  JSI_PROPERTY_GETTER_DECL(positionZ);
  JSI_PROPERTY_GETTER_DECL(forwardX);
  JSI_PROPERTY_GETTER_DECL(forwardY);
  JSI_PROPERTY_GETTER_DECL(forwardZ);
  JSI_PROPERTY_GETTER_DECL(upX);
  JSI_PROPERTY_GETTER_DECL(upY);
  JSI_PROPERTY_GETTER_DECL(upZ);

  [[nodiscard]] AudioListener *audioListener() const {
    return listener_.get();
  }

 private:
  std::unique_ptr<AudioListener> listener_;

  std::shared_ptr<AudioParamHostObject> positionXParam_;
  std::shared_ptr<AudioParamHostObject> positionYParam_;
  std::shared_ptr<AudioParamHostObject> positionZParam_;
  std::shared_ptr<AudioParamHostObject> forwardXParam_;
  std::shared_ptr<AudioParamHostObject> forwardYParam_;
  std::shared_ptr<AudioParamHostObject> forwardZParam_;
  std::shared_ptr<AudioParamHostObject> upXParam_;
  std::shared_ptr<AudioParamHostObject> upYParam_;
  std::shared_ptr<AudioParamHostObject> upZParam_;
};

} // namespace audioapi
