#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/TypedAudioNodePtr.h>
#include <audioapi/HostObjects/destinations/AudioDestinationNodeHostObject.h>
#include <audioapi/HostObjects/utils/JsEnumParser.h>
#include <audioapi/core/AudioNode.h>
#include <audioapi/core/utils/graph/HostGraph.h>
#include <audioapi/jsi/JsiUtils.h>

#include <memory>
#include <utility>

namespace audioapi {
namespace {

bool isMissingEdge(const utils::graph::HostNode::Res &result) {
  if (result.is_ok()) {
    return false;
  }
  const auto err = result.unwrap_err();
  return err == utils::graph::HostGraph::ResultError::EDGE_NOT_FOUND ||
      err == utils::graph::HostGraph::ResultError::NODE_NOT_FOUND;
}

constexpr const char *kDisconnectNotConnected =
    "Failed to execute 'disconnect' on 'AudioNode': the given destination is not connected.";

/// Runs `tryRemove` for each candidate edge. Spec: remove matching edges; throw
/// InvalidAccessError if none existed (or on unexpected graph errors).
template <typename TryRemove>
  requires requires(TryRemove &&tryRemove) {
    tryRemove([](const utils::graph::HostNode::Res &) {});
  }
void disconnectMatchingEdges(jsi::Runtime &runtime, TryRemove &&tryRemove) {
  bool removedAny = false;
  tryRemove([&](const utils::graph::HostNode::Res &result) {
    if (result.is_ok()) {
      removedAny = true;
    } else if (!isMissingEdge(result)) {
      jsiutils::throwException(runtime, "InvalidAccessError", kDisconnectNotConnected);
    }
  });
  if (!removedAny) {
    jsiutils::throwException(runtime, "InvalidAccessError", kDisconnectNotConnected);
  }
}

} // namespace

AudioNodeHostObject::AudioNodeHostObject(
    const std::shared_ptr<utils::graph::Graph> &graph,
    std::unique_ptr<AudioNode> node,
    const AudioNodeOptions &options)
    : utils::graph::HostNode(graph, std::move(node)),
      audioNode_(typedAudioNode<AudioNode>(node_)),
      numberOfInputs_(options.numberOfInputs),
      numberOfOutputs_(options.numberOfOutputs),
      channelCount_(options.channelCount),
      channelCountMode_(options.channelCountMode),
      channelInterpretation_(options.channelInterpretation) {
  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(AudioNodeHostObject, numberOfInputs),
      JSI_EXPORT_PROPERTY_GETTER(AudioNodeHostObject, numberOfOutputs),
      JSI_EXPORT_PROPERTY_GETTER(AudioNodeHostObject, channelCount),
      JSI_EXPORT_PROPERTY_GETTER(AudioNodeHostObject, channelCountMode),
      JSI_EXPORT_PROPERTY_GETTER(AudioNodeHostObject, channelInterpretation));

  addSetters(
      JSI_EXPORT_PROPERTY_SETTER(AudioNodeHostObject, channelCount),
      JSI_EXPORT_PROPERTY_SETTER(AudioNodeHostObject, channelCountMode),
      JSI_EXPORT_PROPERTY_SETTER(AudioNodeHostObject, channelInterpretation));

  addFunctions(
      JSI_EXPORT_FUNCTION(AudioNodeHostObject, connect),
      JSI_EXPORT_FUNCTION(AudioNodeHostObject, disconnect));
}

// Explicitly define destructor here, as they to exist in order to act as a
// "key function" for the audio classes - this allow for RTTI to work
// properly across dynamic library boundaries (i.e. dynamic_cast that is used by
// isHostObject method), android specific issue
AudioNodeHostObject::~AudioNodeHostObject() = default;

JSI_PROPERTY_GETTER_IMPL(AudioNodeHostObject, numberOfInputs) {
  return {numberOfInputs_};
}

JSI_PROPERTY_GETTER_IMPL(AudioNodeHostObject, numberOfOutputs) {
  return {numberOfOutputs_};
}

JSI_PROPERTY_GETTER_IMPL(AudioNodeHostObject, channelCount) {
  return {static_cast<int>(channelCount_)};
}

JSI_PROPERTY_GETTER_IMPL(AudioNodeHostObject, channelCountMode) {
  return jsi::String::createFromUtf8(
      runtime, js_enum_parser::channelCountModeToString(channelCountMode_));
}

JSI_PROPERTY_GETTER_IMPL(AudioNodeHostObject, channelInterpretation) {
  return jsi::String::createFromUtf8(
      runtime, js_enum_parser::channelInterpretationToString(channelInterpretation_));
}

std::shared_ptr<utils::graph::HostNode> AudioNodeHostObject::getOutput(int /*outputIndex*/) {
  return shared_from_this();
}

std::shared_ptr<utils::graph::HostNode> AudioNodeHostObject::getInput(int /*inputIndex*/) {
  return shared_from_this();
}

JSI_PROPERTY_SETTER_IMPL(AudioNodeHostObject, channelCount) {
  const auto count = value.getNumber();
  // Per spec channelCount must be a positive integer; the JS layer throws the
  // DOM error. Guard defensively here — the `!(count >= 1)` form also rejects
  // NaN (NaN compares false to everything).
  if (!(count >= 1)) {
    return;
  }

  updateChannelCount(static_cast<size_t>(count));
}

void AudioNodeHostObject::updateChannelCount(size_t newChannelCount) {
  if (newChannelCount == channelCount_) {
    return;
  }
  // Negotiation reads channelCount on the host thread, so set it here (not on
  // the audio thread) before recomputing layouts.
  audioNode_->setChannelCount(newChannelCount);
  channelCount_ = newChannelCount;
  renegotiate();
}

JSI_PROPERTY_SETTER_IMPL(AudioNodeHostObject, channelCountMode) {
  ChannelCountMode parsedMode;
  try {
    parsedMode = js_enum_parser::channelCountModeFromString(value.toString(runtime).utf8(runtime));
  } catch (const std::invalid_argument &) {
    // WebIDL coerces non-strings; unknown enum values are ignored.
    return;
  }

  if (parsedMode == channelCountMode_) {
    return;
  }

  // channelCountMode is read only on the host thread during negotiation.
  audioNode_->setChannelCountMode(parsedMode);
  channelCountMode_ = parsedMode;
  renegotiate();
}

JSI_PROPERTY_SETTER_IMPL(AudioNodeHostObject, channelInterpretation) {
  ChannelInterpretation parsedInterpretation;
  try {
    parsedInterpretation =
        js_enum_parser::channelInterpretationFromString(value.toString(runtime).utf8(runtime));
  } catch (const std::invalid_argument &) {
    // WebIDL coerces non-strings; unknown enum values are ignored.
    return;
  }

  // channelInterpretation is read on the audio thread (up/down-mix summing in
  // processInputs), so apply it there via a scheduled event. It does not change
  // the channel count, so no renegotiation is needed. Capture the NodeHandle to
  // keep the payload alive until the deferred event runs.
  audioNode_->scheduleAudioEvent(
      [handle = node_->handle, node = audioNode_, parsedInterpretation](BaseAudioContext &) {
        node->setChannelInterpretation(parsedInterpretation);
      });
  channelInterpretation_ = parsedInterpretation;
}

JSI_HOST_FUNCTION_IMPL(AudioNodeHostObject, connect) {
  auto obj = args[0].getObject(runtime);

  const int output = (count > 1 && args[1].isNumber()) ? static_cast<int>(args[1].getNumber()) : 0;
  const int input = (count > 2 && args[2].isNumber()) ? static_cast<int>(args[2].getNumber()) : 0;

  if (output >= numberOfOutputs_) {
    throw jsi::JSError(runtime, "IndexSizeError: connect() output index out of bounds");
  }

  if (obj.isHostObject<AudioNodeHostObject>(runtime)) {
    auto toNodeHost = obj.getHostObject<AudioNodeHostObject>(runtime);

    if (input >= toNodeHost->numberOfInputs_) {
      throw jsi::JSError(runtime, "IndexSizeError: connect() input index out of bounds");
    }

    auto source = getOutput(output);
    auto destination = toNodeHost->getInput(input);
    source->connect(*destination);
  } else if (obj.isHostObject<AudioParamHostObject>(runtime)) {
    auto param = obj.getHostObject<AudioParamHostObject>(runtime);
    param->connectToGraph();
    graph_->addEdge(getOutput(output)->rawNode(), param->bridgeNode());
  }
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioNodeHostObject, disconnect) {
  // disconnect() — drop every outgoing edge from every output.
  if (args == nullptr || count == 0 || args[0].isUndefined()) {
    for (int output = 0; output < numberOfOutputs_; ++output) {
      getOutput(output)->disconnect();
    }
    return jsi::Value::undefined();
  }

  // disconnect(output) — drop every outgoing edge from a single output.
  if (args[0].isNumber()) {
    const int output = static_cast<int>(args[0].getNumber());
    if (output < 0 || output >= numberOfOutputs_) {
      throw jsi::JSError(runtime, "IndexSizeError: disconnect() output index out of bounds");
    }
    getOutput(output)->disconnect();
    return jsi::Value::undefined();
  }

  auto obj = args[0].getObject(runtime);
  const bool hasOutput = count > 1 && args[1].isNumber();
  const bool hasInput = count > 2 && args[2].isNumber();
  const int output = hasOutput ? static_cast<int>(args[1].getNumber()) : 0;
  const int input = hasInput ? static_cast<int>(args[2].getNumber()) : 0;

  if (hasOutput && (output < 0 || output >= numberOfOutputs_)) {
    throw jsi::JSError(runtime, "IndexSizeError: disconnect() output index out of bounds");
  }

  const int outputBegin = hasOutput ? output : 0;
  const int outputEnd = hasOutput ? output + 1 : numberOfOutputs_;

  if (obj.isHostObject<AudioNodeHostObject>(runtime)) {
    auto toNodeHost = obj.getHostObject<AudioNodeHostObject>(runtime);

    if (hasInput && (input < 0 || input >= toNodeHost->numberOfInputs_)) {
      throw jsi::JSError(runtime, "IndexSizeError: disconnect() input index out of bounds");
    }

    const int inputBegin = hasInput ? input : 0;
    const int inputEnd = hasInput ? input + 1 : toNodeHost->numberOfInputs_;

    disconnectMatchingEdges(runtime, [&](auto onResult) {
      for (int o = outputBegin; o < outputEnd; ++o) {
        auto source = getOutput(o);
        for (int i = inputBegin; i < inputEnd; ++i) {
          onResult(source->disconnect(*toNodeHost->getInput(i)));
        }
      }
    });
  } else if (obj.isHostObject<AudioParamHostObject>(runtime)) {
    auto param = obj.getHostObject<AudioParamHostObject>(runtime);
    disconnectMatchingEdges(runtime, [&](auto onResult) {
      for (int o = outputBegin; o < outputEnd; ++o) {
        onResult(graph_->removeEdge(getOutput(o)->rawNode(), param->bridgeNode()));
      }
    });
  }

  return jsi::Value::undefined();
}
} // namespace audioapi
