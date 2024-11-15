#pragma once

#include <memory>
#include <string>
#include <vector>

namespace audioapi {

class BaseAudioContext;
class AudioBus;

class AudioNode : public std::enable_shared_from_this<AudioNode> {
  public:
    explicit AudioNode(BaseAudioContext *context);
    virtual ~AudioNode();
    int getNumberOfInputs() const;
    int getNumberOfOutputs() const;
    int getChannelCount() const;
    std::string getChannelCountMode() const;
    std::string getChannelInterpretation() const;
    void connect(const std::shared_ptr<AudioNode> &node);
    void disconnect(const std::shared_ptr<AudioNode> &node);

  protected:
    enum class ChannelCountMode { MAX, CLAMPED_MAX, EXPLICIT };
    enum class ChannelInterpretation { SPEAKERS, DISCRETE };

    static std::string toString(ChannelCountMode mode);
    static std::string toString(ChannelInterpretation interpretation);

  protected:
    BaseAudioContext *context_;
    std::unique_ptr<AudioBus> audioBus_;

    int numberOfInputs_ = 1;
    int numberOfOutputs_ = 1;
    int channelCount_ = CHANNEL_COUNT;

    ChannelCountMode channelCountMode_ = ChannelCountMode::MAX;
    ChannelInterpretation channelInterpretation_ =
        ChannelInterpretation::SPEAKERS;

    std::vector<std::shared_ptr<AudioNode>> inputNodes_ = {};
    std::vector<std::shared_ptr<AudioNode>> outputNodes_ = {};

    bool processAudio(AudioBus *outputBus, int framesToProcess);

  private:
    void cleanup();
};

} // namespace audioapi
