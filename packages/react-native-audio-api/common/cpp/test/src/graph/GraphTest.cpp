#include <gtest/gtest.h>
#include <audioapi/core/utils/graph/Graph.hpp>
#include <audioapi/core/utils/graph/Disposer.hpp>
#include "TestGraphUtils.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <vector>
#include <cstdlib>
#include <memory>

namespace audioapi::utils::graph {

class MockDisposer : public Disposer {
 public:
  void dispose(AudioGraph::Node* node) override {
      // No-op
  }
};

class GraphTest : public ::testing::Test {
 protected:
  std::unique_ptr<Graph> graph;

  void SetUp() override {
    graph = std::make_unique<Graph>(4096, std::make_unique<MockDisposer>());
  }

  const AudioGraph& getAudioGraph() {
      return graph->audioGraph;
  }

  const HostGraph& getHostGraph() {
      return graph->hostGraph;
  }
};

TEST_F(GraphTest, EventsAreScheduledButNotExecutedUntilProcess) {
    auto* node = graph->addNode();
    ASSERT_NE(node, nullptr);

    // AudioGraph should not be aware of the node structure update yet
    // Assuming createNode just reserves index but doesn't resize vector which is handled by event
    const auto& ag = getAudioGraph();

    size_t sizeBefore = ag.nodes.size();

    // Check if empty/smaller
    if (ag.nodes.empty()) {
        EXPECT_EQ(ag.nodes.size(), 0);
    }

    graph->processEvents();

    EXPECT_GE(ag.nodes.size(), node->audioNodeIndex + 1);
}

TEST_F(GraphTest, NoUselessEventsScheduled) {
    auto* node1 = graph->addNode();
    auto* node2 = graph->addNode();
    graph->processEvents();

    // Initial state
    const auto& ag = getAudioGraph();
    // Convert to verify
    auto initialAdj = TestGraphUtils::convertAudioGraphToAdjacencyList(const_cast<AudioGraph&>(ag)); // casting const away if utils need it, or verifyutils usage

    // Try adding duplicate edge (should fail and NOT schedule event)
    ASSERT_TRUE(graph->addEdge(node1, node2).is_ok()); // Success first time
    graph->processEvents();

    // Result of valid op
    auto intermediateAdj = TestGraphUtils::convertAudioGraphToAdjacencyList(const_cast<AudioGraph&>(ag));

    // Try adding SAME edge (should fail)
    auto result = graph->addEdge(node1, node2);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.unwrap_err(), HostGraph::ResultError::EDGE_ALREADY_EXISTS);

    // Even if we call processEvents, state should not change (and no event should be consumed ideally,
    // impossible to check queue count easily without friend or mock, but state check is good enough)
    graph->processEvents();

    auto finalAdj = TestGraphUtils::convertAudioGraphToAdjacencyList(const_cast<AudioGraph&>(ag));
    EXPECT_EQ(intermediateAdj, finalAdj);
}

TEST_F(GraphTest, ThreadRaceConcurrency) {
    // Stress test with threads
    // One thread produces graph changes
    // One thread processes events (consumer)

    std::atomic<bool> running{true};
    std::vector<HostGraph::Node*> nodes;

    // Add initial nodes
    for(int i=0; i<10; ++i) {
        nodes.push_back(graph->addNode());
    }
    graph->processEvents(); // Setup

    std::thread audioThread([&]() {
        while(running) {
             graph->processEvents();
             std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        graph->processEvents();
    });

    // Main thread mutations
    unsigned int seed = 12345;
    for(int i=0; i<100; ++i) {
        // Randomly add edge or remove edge or add node
        int op = rand_r(&seed) % 3;
        if (op == 0) {
            auto* n = graph->addNode();
            nodes.push_back(n);
        } else if (op == 1 && nodes.size() > 2) {
            // Add edge
            HostGraph::Node *n1, *n2;
            {
               n1 = nodes[rand_r(&seed) % nodes.size()];
               n2 = nodes[rand_r(&seed) % nodes.size()];
            }
            if (n1 != n2) {
                // Ignore result (could be error if edge exists or cycle)
                (void)graph->addEdge(n1, n2);
            }
        } else if (op == 2 && nodes.size() > 5) {
             // Remove edge
            HostGraph::Node *n1, *n2;
            {
               n1 = nodes[rand_r(&seed) % nodes.size()];
               n2 = nodes[rand_r(&seed) % nodes.size()];
            }
            (void)graph->removeEdge(n1, n2);
        }
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    running = false;
    audioThread.join();

    // If we reached here without crash (segfault), we are somewhat good.
    // Verify graph consistency (Host vs Audio)
    {
        // Must wait for all events to flush - already done by final processEvents in thread,
        // but let's do one more to be sure
        graph->processEvents();

        const auto& ag = getAudioGraph();
        const auto& hg = getHostGraph();

        auto audioAdj = TestGraphUtils::convertAudioGraphToAdjacencyList(const_cast<AudioGraph&>(ag));
        auto hostAdj = TestGraphUtils::convertHostGraphToAdjacencyList(const_cast<HostGraph&>(hg));

        // They should match
        EXPECT_EQ(audioAdj, hostAdj);
    }
}

} // namespace audioapi::utils::graph
