#pragma once

#include <jsi/jsi.h>
#include <memory>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#define JSI_HOST_FUNCTION(NAME)  \
  jsi::Value NAME(               \
      jsi::Runtime &runtime,     \
      const jsi::Value &thisVal, \
      const jsi::Value *args,    \
      size_t count)

#define JSI_EXPORT_FUNCTION(CLASS, FUNCTION)                                \
  std::make_pair(                                                           \
      std::string(#FUNCTION),                                               \
      static_cast<jsi::Value (JsiHostObject::*)(                            \
          jsi::Runtime &, const jsi::Value &, const jsi::Value *, size_t)>( \
          &CLASS::FUNCTION))

#define JSI_PROPERTY_GETTER(name) jsi::Value name(jsi::Runtime &runtime)

#define JSI_EXPORT_PROPERTY_GETTER(CLASS, FUNCTION)               \
  std::make_pair(                                                 \
      std::string(#FUNCTION),                                     \
      static_cast<jsi::Value (JsiHostObject::*)(jsi::Runtime &)>( \
          &CLASS::FUNCTION))

#define JSI_PROPERTY_SETTER(name) \
  void name(jsi::Runtime &runtime, const jsi::Value &value)

#define JSI_EXPORT_PROPERTY_SETTER(CLASS, FUNCTION) \
  std::make_pair(                                   \
      std::string(#FUNCTION),                       \
      static_cast<void (JsiHostObject::*)(          \
          jsi::Runtime &, const jsi::Value &)>(&CLASS::FUNCTION))

namespace audioapi {

using namespace facebook;

class JsiHostObject : public jsi::HostObject {
 public:
  JsiHostObject();

  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;

  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;

  template <typename... Args>
  void addGetters(Args... args) {
    (propertyGetters_->insert(args), ...);
  }

  template <typename... Args>
  void addSetters(Args... args) {
    (propertySetters_->insert(args), ...);
  }

  template <typename... Args>
  void addFunctions(Args... args) {
    (propertyFunctions_->insert(args), ...);
  }

  JSI_PROPERTY_GETTER(then) {
    return jsi::Value::undefined();
  }

 protected:
  std::unique_ptr<std::unordered_map<
      std::string,
      jsi::Value (JsiHostObject::*)(jsi::Runtime &)>>
      propertyGetters_;

  std::unique_ptr<std::unordered_map<
      std::string,
      jsi::Value (JsiHostObject::*)(
          jsi::Runtime &,
          const jsi::Value &,
          const jsi::Value *,
          size_t)>>
      propertyFunctions_;

  std::unique_ptr<std::unordered_map<
      std::string,
      void (JsiHostObject::*)(jsi::Runtime &, const jsi::Value &)>>
      propertySetters_;
};

} // namespace audioapi
