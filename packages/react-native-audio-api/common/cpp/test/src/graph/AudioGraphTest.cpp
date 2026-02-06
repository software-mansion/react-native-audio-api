#include <gtest/gtest.h>
#include <audioapi/core/utils/graph/AudioGraph.h>
#include <audioapi/core/utils/graph/HostGraph.h>
#include <utility>
#include <unordered_map>
#include <vector>

namespace audioapi::utils::graph {

class AudioGraphTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }

  // Helper to create a graph with given identifiers
 std::pair<AudioGraph::Node*, std::unordered_map<size_t, AudioGraph::Node*>> createGraphNodes(std::vector<size_t> ids) {
    if (ids.empty()) return {nullptr, {}};

    AudioGraph::Node* head = new AudioGraph::Node();
    std::unordered_map<size_t, AudioGraph::Node*> nodeMap;
    nodeMap[ids[0]] = head;
    head->test_node_identifier__ = ids[0];
    AudioGraph::Node* tail = head;

    for (size_t i = 1; i < ids.size(); ++i) {
      AudioGraph::Node* newNode = new AudioGraph::Node();
      newNode->test_node_identifier__ = ids[i];
      nodeMap[ids[i]] = newNode;
      tail->next = newNode;
      newNode->prev = tail;
      tail = newNode;
    }

    return {head, nodeMap};
  }

  void setGraphHead(AudioGraph& graph, AudioGraph::Node* head) {
    graph.head = head;
  }

  void swapNodes(AudioGraph& graph, AudioGraph::Node* a, AudioGraph::Node* b) {
    graph.swapNodesInTopologicalOrder(a, b);
  }

  AudioGraph::Node* getGraphHead(AudioGraph& graph) {
      return graph.head;
  }

  void verifyGraphOrder(AudioGraph& graph, std::vector<size_t> expectedIds) {
    AudioGraph::Node* current = getGraphHead(graph);
    AudioGraph::Node* prev = nullptr;

    for (size_t i = 0; i < expectedIds.size(); ++i) {
      size_t id = expectedIds[i];
      ASSERT_NE(current, nullptr) << "Expected node with id " << id << " but reached end of graph at index " << i;
      EXPECT_EQ(current->test_node_identifier__, id) << "Mismatch at index " << i;
      EXPECT_EQ(current->prev, prev) << "Prev pointer broken at node " << id;

      prev = current;
      current = current->next;
    }
    EXPECT_EQ(current, nullptr) << "Graph has more nodes than expected";
  }
};


TEST_F(AudioGraphTest, swapAdjacent_A_B_Middle) {
  // A -> B
  auto [headNode, nodeMap] = createGraphNodes({1, 2, 3, 4});
  AudioGraph graph;
  setGraphHead(graph, headNode);

  // Swap 2 and 3
  swapNodes(graph, nodeMap[2], nodeMap[3]);

  verifyGraphOrder(graph, {1, 3, 2, 4});
}

TEST_F(AudioGraphTest, swapAdjacent_B_A_Middle) {
  // B -> A (Swapping 2 and 3 by passing (3, 2) instead of (2,3))
  auto [headNode, nodeMap] = createGraphNodes({1, 2, 3, 4});
  AudioGraph graph;
  setGraphHead(graph, headNode);

  // Swap 3 and 2 (where 2 is before 3 in graph)
  // Calling swap with (3, 2) should behave same as (2, 3) effectively swapping their positions
  swapNodes(graph, nodeMap[3], nodeMap[2]);

  verifyGraphOrder(graph, {1, 3, 2, 4});
}

TEST_F(AudioGraphTest, swapAdjacent_Head_Next) {
  auto [headNode, nodeMap] = createGraphNodes({1, 2, 3});
  AudioGraph graph;
  setGraphHead(graph, headNode);

  // Swap 1 and 2 (Head and Next)
  swapNodes(graph, nodeMap[1], nodeMap[2]);

  verifyGraphOrder(graph, {2, 1, 3});
}

TEST_F(AudioGraphTest, swapAdjacent_TailPrev_Tail) {
  auto [headNode, nodeMap] = createGraphNodes({1, 2, 3});
  AudioGraph graph;
  setGraphHead(graph, headNode);

  // Swap 2 and 3 (TailPrev and Tail)
  swapNodes(graph, nodeMap[2], nodeMap[3]);

  verifyGraphOrder(graph, {1, 3, 2});
}

TEST_F(AudioGraphTest, swapNonAdjacent_Middle) {
  auto [headNode, nodeMap] = createGraphNodes({1, 2, 3, 4, 5});
  AudioGraph graph;
  setGraphHead(graph, headNode);

  // Swap 2 and 4 (Separated by 3)
  swapNodes(graph, nodeMap[2], nodeMap[4]);

  verifyGraphOrder(graph, {1, 4, 3, 2, 5});
}

TEST_F(AudioGraphTest, swapNonAdjacent_Head_Tail) {
  auto [headNode, nodeMap] = createGraphNodes({1, 2, 3, 4});
  AudioGraph graph;
  setGraphHead(graph, headNode);

  // Swap 1 and 4
  swapNodes(graph, nodeMap[1], nodeMap[4]);

  verifyGraphOrder(graph, {4, 2, 3, 1});
}

TEST_F(AudioGraphTest, swapNonAdjacent_Head_Middle) {
  auto [headNode, nodeMap] = createGraphNodes({1, 2, 3, 4});
  AudioGraph graph;
  setGraphHead(graph, headNode);

  // Swap 1 and 3
  swapNodes(graph, nodeMap[1], nodeMap[3]);

  verifyGraphOrder(graph, {3, 2, 1, 4});
}

TEST_F(AudioGraphTest, swapNonAdjacent_Middle_Tail) {
  auto [headNode, nodeMap] = createGraphNodes({1, 2, 3, 4});
  AudioGraph graph;
  setGraphHead(graph, headNode);

  // Swap 2 and 4
  swapNodes(graph, nodeMap[2], nodeMap[4]);

  verifyGraphOrder(graph, {1, 4, 3, 2});
}

TEST_F(AudioGraphTest, IteratorTest) {
  auto [headNode, nodeMap] = createGraphNodes({10, 20, 30});
  AudioGraph graph;
  setGraphHead(graph, headNode);

  AudioGraph::Iterator it(getGraphHead(graph));

  AudioGraph::Node* node = it.next();
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->test_node_identifier__, 10);

  node = it.next();
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->test_node_identifier__, 20);

  node = it.next();
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->test_node_identifier__, 30);

  node = it.next();
  EXPECT_EQ(node, nullptr);
}

}; // namespace audioapi::utils::graph
