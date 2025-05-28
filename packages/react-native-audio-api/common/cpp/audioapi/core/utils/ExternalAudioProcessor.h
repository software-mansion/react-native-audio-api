#pragma once

#include <memory>

namespace audioapi {

/**
 * @brief Interface for external audio processing modules.
 *
 * This abstract base class allows developers to define custom DSP (digital signal processing)
 * logic that operates on raw audio buffers. Implementations of this interface can be registered
 * dynamically and integrated into the audio rendering pipeline.
 *
 * By using raw buffer pointers (`float**`), this interface avoids coupling with internal engine
 * structures like AudioBus, making it flexible and portable across different backends.
 */
class ExternalAudioProcessor {
public:
  virtual ~ExternalAudioProcessor() = default;

  /**
   * @brief Processes a block of audio data.
   *
   * @param channelData An array of pointers to audio channel buffers.
   *        Each pointer points to a float array of frame samples.
   * @param numChannels The number of active audio channels.
   * @param numFrames The number of frames (samples) per channel to process.
   *
   * Implementations may modify the buffer contents in-place to apply custom effects,
   * transformations, analysis, etc.
   */
  virtual void process(float** channelData, int numChannels, int numFrames) = 0;
};

/**
 * @brief Singleton registry for managing an active external audio processor.
 *
 * This class acts as a global hook into the audio engine, allowing external modules to register
 * a processor implementation at runtime. The audio system can then query the registry and
 * invoke the registered processor, if available, during audio rendering.
 *
 * This registry ensures that only one active processor exists at a time.
 */
class ExternalAudioProcessorRegistry {
public:
  /**
   * @brief Returns the singleton instance of the registry.
   */
  static ExternalAudioProcessorRegistry& getInstance();

  /**
   * @brief Registers a new external audio processor.
   *
   * Replaces any previously registered processor.
   *
   * @param processor A shared pointer to the processor implementation.
   *                  Pass `nullptr` to clear and unregister the current processor.
   */
  void registerProcessor(std::shared_ptr<ExternalAudioProcessor> processor);

  /**
   * @brief Retrieves the currently registered processor.
   *
   * @return Shared pointer to the processor, or nullptr if none is registered.
   */
  std::shared_ptr<ExternalAudioProcessor> getProcessor();

private:
  ExternalAudioProcessorRegistry() = default;

  std::shared_ptr<ExternalAudioProcessor> processor_;
};

} // namespace audioapi