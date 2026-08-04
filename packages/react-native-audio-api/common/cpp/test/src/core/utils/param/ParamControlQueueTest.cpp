#include <audioapi/core/types/ParamEventType.h>
#include <audioapi/core/utils/param/ParamControlQueue.h>
#include <audioapi/core/utils/param/ParamEvent.h>
#include <gtest/gtest.h>

namespace audioapi::test {

TEST(ParamControlQueueTest, TrivialConstructAndPurge) {
  ParamControlQueue queue;
  ParamEvent event(ParamEventType::SET_VALUE, 0.0);
  EXPECT_TRUE(queue.checkCurveExclusion(event).is_ok());
  queue.purge(0.0);
}

} // namespace audioapi::test
