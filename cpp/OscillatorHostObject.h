#ifndef OSCILLATORHOSTOBJECT_H
#define OSCILLATORHOSTOBJECT_H

#include <jsi/jsi.h>
#include <PlatformOscillator.h>

namespace audiocontext
{

  using namespace facebook;

  class JSI_EXPORT OscillatorHostObject : public jsi::HostObject
  {
    public:
      explicit OscillatorHostObject(float frequency) : frequency_(frequency), platformOscillator_(frequency) {}
      jsi::Value get(jsi::Runtime &, const jsi::PropNameID &name) override;
      void set(jsi::Runtime &, const jsi::PropNameID &name, const jsi::Value &value) override;
      std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

    protected:
      PlatformOscillator platformOscillator_;
      float frequency_;
  };

} // namespace audiocontext

#endif /* OSCILLATORHOSTOBJECT_H */
