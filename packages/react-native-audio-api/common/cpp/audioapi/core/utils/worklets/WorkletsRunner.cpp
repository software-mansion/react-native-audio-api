#include <audioapi/core/utils/worklets/WorkletsRunner.h>

namespace audioapi {

WorkletsRunner::WorkletsRunner(
    std::weak_ptr<worklets::WorkletRuntime> weakUiRuntime) noexcept
    : weakUiRuntime_(std::move(weakUiRuntime)) {}

}; // namespace audioapi
