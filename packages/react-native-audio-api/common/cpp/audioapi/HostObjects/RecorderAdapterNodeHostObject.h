#pragma once

#include <audioapi/HostObjects/AudioScheduledSourceNodeHostObject.h>
#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/HostObjects/AudioRecorderHostObject.h>

#include <memory>
#include <string>
#include <vector>

namespace audioapi {
using namespace facebook;

class RecorderAdapterNodeHostObject : public AudioScheduledSourceNodeHostObject {
 public:
    explicit RecorderAdapterNodeHostObject(
        const std::shared_ptr<RecorderAdapterNode> &node)
        : AudioScheduledSourceNodeHostObject(node) {
        addFunctions(
            JSI_EXPORT_FUNCTION(RecorderAdapterNodeHostObject, setRecorder));
        }

    JSI_HOST_FUNCTION(setRecorder) {
        auto recorderHostObject = args[0].getObject(runtime).getHostObject<AudioRecorderHostObject>(runtime);
        auto recorderAdapterNode =
            std::static_pointer_cast<RecorderAdapterNode>(node_);
        recorderAdapterNode->setRecorder(std::shared_ptr<AudioRecorder>(recorderHostObject->audioRecorder_));
        return jsi::Value::undefined();
    }
};

} // namespace audioapi
