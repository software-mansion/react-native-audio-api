#include <audioapi/core/utils/worklets/UiWorkletsRunner.h>

namespace audioapi {

UiWorkletsRunner::UiWorkletsRunner(
    std::weak_ptr<worklets::WorkletRuntime> weakUiRuntime) noexcept
    : weakUiRuntime_(std::move(weakUiRuntime)) {}

}; // namespace audioapi
