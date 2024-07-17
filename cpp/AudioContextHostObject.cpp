#include <AudioContextHostObject.h>
#include <OscillatorNodeHostObject.h>
#include <jsi/jsi.h>

namespace audiocontext {
	using namespace facebook;

  std::vector<jsi::PropNameID> AudioContextHostObject::getPropertyNames(jsi::Runtime &runtime)
  {
    std::vector<jsi::PropNameID> propertyNames;

    return propertyNames;
  }

  jsi::Value AudioContextHostObject::get(jsi::Runtime &runtime, const jsi::PropNameID &propNameId) {
    auto propName = propNameId.utf8(runtime);

		if(propName == "createOscillator") {
      return createOscillator(runtime, propNameId);
    }

    throw std::runtime_error("Not yet implemented!");
  }

  void AudioContextHostObject::set(jsi::Runtime &runtime, const jsi::PropNameID &propNameId, const jsi::Value &value)
  {
    auto propName = propNameId.utf8(runtime);

    throw std::runtime_error("Not yet implemented!");
  }

	jsi::Value AudioContextHostObject::createOscillator(jsi::Runtime &runtime, const jsi::PropNameID &propNameId) {
        return jsi::Function::createFromHostFunction(runtime, propNameId, 1,
				[](jsi::Runtime& runtime, const jsi::Value& thisVal, const jsi::Value* args, size_t count) -> jsi::Value {
					const float frequency = static_cast<float>(args[0].asNumber());

					auto cppOscillatorNodeHostObjectPtr = std::make_shared<audiocontext::OscillatorNodeHostObject>(frequency);

					const auto& jsiOscillatorNodeHostObject = jsi::Object::createFromHostObject(runtime, cppOscillatorNodeHostObjectPtr);

					return jsi::Value(runtime, jsiOscillatorNodeHostObject);
			});
    }
}