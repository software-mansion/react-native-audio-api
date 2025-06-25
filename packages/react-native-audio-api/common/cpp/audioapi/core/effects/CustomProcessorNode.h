#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>
#include <memory>
#include <string>
#include <cstring>
#include <functional>
#include <map>
#include <unordered_map>
#include <vector>
#include <any>
#include <utility>

namespace audioapi {

class AudioBus;

/**
 * @brief Interface for defining custom audio processors that can be attached to nodes.
 */
class CustomAudioProcessor {
 public:
  virtual ~CustomAudioProcessor() = default;

  /**
   * @brief Processes audio data in-place.
   * @param channelData Array of float pointers for each channel.
   * @param numChannels Number of audio channels.
   * @param numFrames Number of frames per channel.
   */
  virtual void processInPlace(float** channelData, int numChannels, int numFrames) {
    // Default: do nothing
  }

  /**
   * @brief Processes audio data from input to output buffers.
   * Default behavior copies input to output and then calls in-place processing.
   * @param input Input audio buffers.
   * @param output Output audio buffers.
   * @param numChannels Number of channels.
   * @param numFrames Number of frames per channel.
   */
  virtual void processThrough(float** input, float** output, int numChannels, int numFrames) {
    for (int ch = 0; ch < numChannels; ++ch) {
      std::memcpy(output[ch], input[ch], sizeof(float) * numFrames);
    }
    processInPlace(output, numChannels, numFrames);
  }
};

/**
 * @brief Audio node that wraps a user-defined processor for custom audio processing.
 */
class CustomProcessorNode : public AudioNode {
 public:
  /**
   * @brief Enumerates supported audio processing modes.
   */
  enum class ProcessorMode {
    ProcessInPlace,
    ProcessThrough
  };

  /**
   * @brief Constructs a new CustomProcessorNode attached to a given context with a processor identifier.
   * @param context Audio context this node belongs to.
   * @param identifier The identifier of the processor to instantiate and bind.
   */
  CustomProcessorNode(BaseAudioContext* context, const std::string& identifier);

  /**
   * @brief Destroys the node and removes it from active tracking.
   */
  ~CustomProcessorNode();

  /**
   * @brief Gets the custom AudioParam associated with this node.
   * @return Shared pointer to an AudioParam.
   */
  std::shared_ptr<AudioParam> getCustomProcessorParam() const;

  /**
   * @brief Gets the processor mode.
   * @return The current processing mode as an enum.
   */
  ProcessorMode getProcessorMode() const;

  /**
   * @brief Sets the processor mode.
   * @param processorMode Processing mode to apply.
   */
  void setProcessorMode(ProcessorMode processorMode);

  /**
   * @brief Registers a factory for creating processors for a given identifier.
   * @param identifier Unique string key.
   * @param factory Function that returns a shared CustomAudioProcessor.
   */
  static void registerProcessorFactory(
      const std::string& identifier,
      std::function<std::shared_ptr<CustomAudioProcessor>()> factory);

  /**
   * @brief Unregisters a processor factory.
   * @param identifier Key to remove.
   */
  static void unregisterProcessorFactory(const std::string& identifier);

  /**
   * @brief Alias for control callbacks accepting arbitrary parameters.
   */
  using GenericControlHandler = std::function<void(const std::vector<std::any>&)>;

  /**
   * @brief Registers a control handler for automation (e.g., gain or filter changes).
   * @param identifier Control name key.
   * @param handler Function to handle dynamic updates.
   */
  static void registerControlHandler(const std::string& identifier, GenericControlHandler handler);

  /**
   * @brief Unregisters a control handler by identifier.
   * @param identifier The key to remove.
   */
  static void unregisterControlHandler(const std::string& identifier);

  /**
   * @brief Invokes a control handler using variadic arguments.
   * @tparam Args Parameter types.
   * @param identifier Key associated with the control handler.
   * @param args Arguments passed to the handler.
   */
  template <typename... Args>
  static void applyControlToNode(const std::string& identifier, Args&&... args) {
    auto it = s_controlHandlersByIdentifier.find(identifier);
    if (it != s_controlHandlersByIdentifier.end()) {
      std::vector<std::any> paramList = { std::forward<Args>(args)... };
      it->second(paramList);
    }
  }

 protected:
  /**
   * @brief Core audio processing callback invoked each render cycle.
   * @param processingBus The audio bus containing buffers.
   * @param framesToProcess Number of frames to process.
   */
  void processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;

 private:
  /**
   * @brief In-place audio processing.
   * @param bus The shared audio bus buffer.
   * @param frames Number of frames to process.
   */
  void processInPlace(const std::shared_ptr<AudioBus>& bus, int frames);

  /**
   * @brief Processes audio using separate input and output buffers.
   * @param bus Audio bus used for both input and output access.
   * @param frames Number of frames to process.
   */
  void processThrough(const std::shared_ptr<AudioBus>& bus, int frames);

  /**
   * @brief Ensures that preallocated output buffers match the required frame and channel dimensions.
   * This avoids real-time heap allocations.
   */
  void ensurePreallocatedBuffers(int numChannels, int numFrames);

  std::shared_ptr<AudioParam> customProcessorParam_; ///< Optional real-time modifiable parameter.
  ProcessorMode processorMode_ = ProcessorMode::ProcessInPlace; ///< Determines processing style.
  std::shared_ptr<CustomAudioProcessor> processor_; ///< Processor instance.

  std::vector<std::vector<float>> preallocatedOutputBuffers_; ///< Storage for preallocated channel data.
  std::vector<float*> preallocatedOutputPointers_; ///< Pointers to preallocatedOutputBuffers_ for processor API.

  static void notifyProcessorChanged(const std::string& identifier); ///< Notify and update all nodes with given identifier.

  static std::map<std::string, std::function<std::shared_ptr<CustomAudioProcessor>()>> s_processorFactoriesByIdentifier; ///< Global registry of processor factories.
  static std::unordered_map<std::string, GenericControlHandler> s_controlHandlersByIdentifier; ///< Global registry of control handlers.
  static std::unordered_map<std::string, std::vector<CustomProcessorNode*>> s_activeNodes; ///< Tracks active node instances by identifier.
};

} // namespace audioapi
