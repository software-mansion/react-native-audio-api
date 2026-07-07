#include "jsi_install.h"

#include <node_api.h>

namespace {

napi_value install(napi_env env, napi_callback_info /* info */) {
  return installUsingJsiBindings(env);
}

napi_value createNativeModule(napi_env env) {
  napi_value module;
  napi_create_object(env, &module);

  napi_value installFn;
  napi_create_function(env, "install", NAPI_AUTO_LENGTH, install, nullptr, &installFn);
  napi_set_named_property(env, module, "install", installFn);

  return module;
}

napi_value Init(napi_env env, napi_value exports) {
  napi_value nativeModule = createNativeModule(env);
  napi_set_named_property(env, exports, "nativeModule", nativeModule);
  return exports;
}

} // namespace

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
