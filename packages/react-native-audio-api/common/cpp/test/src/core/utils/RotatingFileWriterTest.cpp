#include <audioapi/core/utils/RotatingFileWriter.h>
#include <gtest/gtest.h>

#include <string>

namespace audioapi::test {

// TODO: replace this testcase with more meaningful ones.
TEST(RotatingFileWriterTest, TrivialConstruct) {
  RotatingFileWriter writer(1024, [](const auto &) { return nullptr; }, [](const std::string &) {});
  SUCCEED();
}

} // namespace audioapi::test
