#ifndef AUDIOCONTEXTHOSTOBJECT_H
#define AUDIOCONTEXTHOSTOBJECT_H

#include <jsi/jsi.h>

namespace audiocontext
{

  using namespace facebook;

  class JSI_EXPORT AudioContextHostObject : public jsi::HostObject
  {
    public:
      explicit AudioContextHostObject() {}
      jsi::Value get(jsi::Runtime &, const jsi::PropNameID &name) override;
      void set(jsi::Runtime &, const jsi::PropNameID &name, const jsi::Value &value) override;
      std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

		protected:
			jsi::Value createOscillator(jsi::Runtime& runtime, const jsi::PropNameID& propNameId);
  };

} // namespace audiocontext

#endif /* OscillatorNodeHostObject_H */
