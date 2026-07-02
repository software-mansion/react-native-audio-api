#include "NodeJSRuntimeApi.h"

#include <cstring>

namespace rnaudioapi::node {

namespace {
using Microsoft::NodeApiJsi::FuncPtr;
} // namespace

NodeJSRuntimeApi::NodeJSRuntimeApi() : api_(&resolver_) {}

Microsoft::NodeApiJsi::JSRuntimeApi *NodeJSRuntimeApi::api() {
  return &api_;
}

FuncPtr NodeJSRuntimeApi::ProcessSymbolResolver::getFuncPtr(const char *funcName) {
#define NODE_API_FUNC(func)                   \
  if (std::strcmp(funcName, #func) == 0) {    \
    return reinterpret_cast<FuncPtr>(&func);  \
  }
#include <ApiLoaders/NodeApi.inc>

  // Returning nullptr for jsr_* is intentional: JSRuntimeApi uses
  // built-in defaults for missing JSR symbols.
  return nullptr;
}

} // namespace rnaudioapi::node
