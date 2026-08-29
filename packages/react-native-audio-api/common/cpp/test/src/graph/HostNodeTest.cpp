#include <audioapi/core/utils/Disposer.hpp>
#include <audioapi/core/utils/graph/Graph.h>
#include <gtest/gtest.h>
#include <test/src/graph/TestGraphUtils.h>

#include <memory>

namespace audioapi::test {

TEST(HostNodeTest, TrivialConstruct) {
  utils::DisposerImpl<audioapi::DISPOSER_PAYLOAD_SIZE> disposer{64};
  auto graph = std::make_shared<utils::graph::Graph>(64, &disposer);
  utils::graph::MockHostNode node(graph);
  EXPECT_NE(node.rawNode(), nullptr);
}

} // namespace audioapi::test
