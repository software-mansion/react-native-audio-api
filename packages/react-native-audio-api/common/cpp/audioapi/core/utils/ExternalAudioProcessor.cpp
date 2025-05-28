#include <audioapi/core/utils/ExternalAudioProcessor.h>

namespace audioapi {

/**
 * @brief Retrieves the global singleton instance of the processor registry.
 *
 * This method ensures that only one instance of the registry exists throughout the application’s lifecycle.
 * It allows any part of the audio system to access and interact with the same registered processor.
 *
 * @return Reference to the singleton ExternalAudioProcessorRegistry.
 */
ExternalAudioProcessorRegistry& ExternalAudioProcessorRegistry::getInstance() {
  static ExternalAudioProcessorRegistry instance;
  return instance;
}

/**
 * @brief Registers a custom external audio processor.
 *
 * Replaces any existing processor with the one provided. The new processor will be called
 * during audio rendering to perform custom DSP operations.
 *
 * To unregister the processor (i.e., disable external processing), pass in `nullptr`.
 *
 * @param processor A shared pointer to the processor implementation, or nullptr to unregister.
 */
void ExternalAudioProcessorRegistry::registerProcessor(std::shared_ptr<ExternalAudioProcessor> processor) {
  processor_ = std::move(processor);
}

/**
 * @brief Returns the currently registered external processor.
 *
 * If no processor is registered, returns `nullptr`.
 *
 * @return Shared pointer to the registered processor, or nullptr.
 */
std::shared_ptr<ExternalAudioProcessor> ExternalAudioProcessorRegistry::getProcessor() {
  return processor_;
}

} // namespace audioapi