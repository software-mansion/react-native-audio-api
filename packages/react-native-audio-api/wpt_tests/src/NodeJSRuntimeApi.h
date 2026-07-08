#pragma once

#include <ApiLoaders/JSRuntimeApi.h>

namespace rnaudioapi::node {

class NodeJSRuntimeApi final {
 public:
  NodeJSRuntimeApi();

  Microsoft::NodeApiJsi::JSRuntimeApi *api();

 private:
  class ProcessSymbolResolver final : public Microsoft::NodeApiJsi::IFuncResolver {
   public:
    Microsoft::NodeApiJsi::FuncPtr getFuncPtr(const char *funcName) override;
  };

  ProcessSymbolResolver resolver_;
  Microsoft::NodeApiJsi::JSRuntimeApi api_;
};

} // namespace rnaudioapi::node
