#pragma once

#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/graph/HostNode.h>
#include <audioapi/core/utils/param/ParamControlQueue.h>
#include <audioapi/jsi/JsiHostObject.h>
#include <audioapi/utils/Result.hpp>
#include <jsi/jsi.h>
#include <cstddef>
#include <memory>
#include <string>

namespace audioapi {
using namespace facebook;

class AudioParam;

/// Rough native footprint of an AudioParamHostObject:
/// two DSPAudioBuffer(RQ) (k-rate + a-rate scratch) + control queue + atomics.
inline constexpr size_t kAudioParamBytes = 2 * RENDER_QUANTUM_SIZE * sizeof(float) + 512;

/// @brief Host object for AudioParam that owns its BridgeNode.
///
/// The BridgeNode is created lazily on the first connectToGraph() call
/// and connected to the owner node (bridge → owner). Sources connecting
/// to this param connect to the bridge node (source → bridge).
///
/// When destroyed, the BridgeNode is removed from the graph.
class AudioParamHostObject : public JsiHostObject {
 public:
  using HNode = utils::graph::HostGraph::Node;

  /// @brief Creates an AudioParamHostObject with its BridgeNode.
  /// @param graph The audio graph
  /// @param ownerNode The HNode* of the AudioNode that owns this param
  /// @param param The AudioParam this host object represents
  explicit AudioParamHostObject(
      std::shared_ptr<utils::graph::Graph> graph,
      HNode *ownerNode,
      const std::shared_ptr<AudioParam> &param);

  ~AudioParamHostObject() override;

  JSI_PROPERTY_GETTER_DECL(value);
  JSI_PROPERTY_GETTER_DECL(defaultValue);
  JSI_PROPERTY_GETTER_DECL(minValue);
  JSI_PROPERTY_GETTER_DECL(maxValue);

  JSI_PROPERTY_SETTER_DECL(value);

  JSI_HOST_FUNCTION_DECL(setValueAtTime);
  JSI_HOST_FUNCTION_DECL(linearRampToValueAtTime);
  JSI_HOST_FUNCTION_DECL(exponentialRampToValueAtTime);
  JSI_HOST_FUNCTION_DECL(setTargetAtTime);
  JSI_HOST_FUNCTION_DECL(setValueCurveAtTime);
  JSI_HOST_FUNCTION_DECL(cancelScheduledValues);
  JSI_HOST_FUNCTION_DECL(cancelAndHoldAtTime);
  JSI_HOST_FUNCTION_DECL(checkCurveExclusion);

  /// @brief Returns the bridge node for this param (for source → bridge connections).
  [[nodiscard]] HNode *bridgeNode() const {
    return bridgeNode_;
  }

  void connectToGraph();

 private:
  friend class AudioNodeHostObject;

  std::shared_ptr<utils::graph::Graph> graph_;
  HNode *ownerNode_ = nullptr;
  HNode *bridgeNode_ = nullptr;
  bool isConnectedToGraph_ = false;
  std::shared_ptr<AudioParam> param_;
  ParamControlQueue controlQueue_;
  float defaultValue_;
  float minValue_;
  float maxValue_;

  Result<NoneType, std::string> checkCurveExclusionFromJSI(
      jsi::Runtime &runtime,
      const jsi::Value *args);
};
} // namespace audioapi
