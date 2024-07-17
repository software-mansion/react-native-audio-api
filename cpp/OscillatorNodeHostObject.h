#ifndef OscillatorNodeHostObject_H
#define OscillatorNodeHostObject_H

#include <jsi/jsi.h>
#include <IOSOscillator.h>

namespace audiocontext
{

  using namespace facebook;

  class JSI_EXPORT OscillatorNodeHostObject : public jsi::HostObject
  {
    public:
      explicit OscillatorNodeHostObject(float frequency) : frequency_(frequency), IOSOscillator_(frequency) {}
      jsi::Value get(jsi::Runtime &, const jsi::PropNameID &name) override;
      void set(jsi::Runtime &, const jsi::PropNameID &name, const jsi::Value &value) override;
      std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

    protected:
      IOSOscillator IOSOscillator_;
      float frequency_;
  };

} // namespace audiocontext

#endif /* OscillatorNodeHostObject_H */
