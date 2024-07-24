#include "GainNodeHostObject.h"

namespace audiocontext
{
  using namespace facebook;

  std::vector<jsi::PropNameID> GainNodeHostObject::getPropertyNames(jsi::Runtime &runtime)
  {
    std::vector<jsi::PropNameID> propertyNames;
    return propertyNames;
  }

  jsi::Value GainNodeHostObject::get(jsi::Runtime &runtime, const jsi::PropNameID &propNameId)
  {
    auto propName = propNameId.utf8(runtime);

    throw std::runtime_error("Prop not yet implemented!");
  }

  void GainNodeHostObject::set(jsi::Runtime &runtime, const jsi::PropNameID &propNameId, const jsi::Value &value)
  {
    auto propName = propNameId.utf8(runtime);
      
    if (propName == "gain") {
        double gain = value.getNumber();
        wrapper_->setGain(gain);
        return;
    }

    throw std::runtime_error("Not yet implemented!");
  }
}
