#include <audioapi/utils/TripleBuffer.hpp>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

using namespace audioapi;

struct IntVal {
  int value{0};
  explicit IntVal(int v = 0) : value(v) {}
};

// NOLINTBEGIN

// ---------------------------------------------------------------------------
// Functional Tests
// ---------------------------------------------------------------------------

class TripleBufferTest : public ::testing::Test {};

TEST_F(TripleBufferTest, GetForWriterReturnsNonNull) {
  TripleBuffer<IntVal> buf;
  EXPECT_NE(buf.getForWriter(), nullptr);
}

TEST_F(TripleBufferTest, GetForReaderReturnsNonNull) {
  TripleBuffer<IntVal> buf;
  EXPECT_NE(buf.getForReader(), nullptr);
}

TEST_F(TripleBufferTest, ConstructorForwardsArguments) {
  TripleBuffer<IntVal> buf(42);
  EXPECT_EQ(buf.getForWriter()->value, 42);
  EXPECT_EQ(buf.getForReader()->value, 42);
}

TEST_F(TripleBufferTest, ReaderAndWriterPointToDifferentBuffers) {
  TripleBuffer<IntVal> buf;
  EXPECT_NE(buf.getForWriter(), buf.getForReader());
}

TEST_F(TripleBufferTest, PublishMakesWrittenDataVisibleToReader) {
  TripleBuffer<IntVal> buf;

  buf.getForWriter()->value = 99;
  buf.publish();

  EXPECT_EQ(buf.getForReader()->value, 99);
}

TEST_F(TripleBufferTest, ReaderReturnsSameBufferWithoutPublish) {
  TripleBuffer<IntVal> buf;

  auto *first = buf.getForReader();
  auto *second = buf.getForReader();
  EXPECT_EQ(first, second);
}

TEST_F(TripleBufferTest, MultiplePublishesWithoutReadShowLatestValue) {
  TripleBuffer<IntVal> buf;

  for (int i = 1; i <= 5; ++i) {
    buf.getForWriter()->value = i;
    buf.publish();
  }

  EXPECT_EQ(buf.getForReader()->value, 5);
}

TEST_F(TripleBufferTest, ReadClearsUpdateFlag) {
  TripleBuffer<IntVal> buf;

  buf.getForWriter()->value = 42;
  buf.publish();

  auto *first = buf.getForReader();
  auto *second = buf.getForReader();

  EXPECT_EQ(first, second);
}

TEST_F(TripleBufferTest, WriterPointerIsStableBeforePublish) {
  TripleBuffer<IntVal> buf;

  auto *w1 = buf.getForWriter();
  auto *w2 = buf.getForWriter();
  EXPECT_EQ(w1, w2);
}

TEST_F(TripleBufferTest, WriterBufferChangesAfterPublish) {
  TripleBuffer<IntVal> buf;

  auto *before = buf.getForWriter();
  buf.publish();
  auto *after = buf.getForWriter();

  EXPECT_NE(before, after);
}

TEST_F(TripleBufferTest, AlternatingWriteReadPreservesData) {
  TripleBuffer<IntVal> buf;

  for (int i = 0; i < 10; ++i) {
    buf.getForWriter()->value = i;
    buf.publish();
    EXPECT_EQ(buf.getForReader()->value, i);
  }
}

TEST_F(TripleBufferTest, ReaderWithNoUpdateSeesInitialValue) {
  TripleBuffer<IntVal> buf(7);
  EXPECT_EQ(buf.getForReader()->value, 7);
}

TEST_F(TripleBufferTest, DestructorCalledExactlyThreeTimes) {
  struct Tracker {
    std::atomic<int> *count;
    explicit Tracker(std::atomic<int> *c) : count(c) {}
    ~Tracker() {
      ++(*count);
    }
  };

  std::atomic<int> count{0};
  { TripleBuffer<Tracker> buf(&count); }
  EXPECT_EQ(count.load(), 3);
}

TEST_F(TripleBufferTest, WriteThenReadSequenceIsConsistent) {
  TripleBuffer<IntVal> buf;

  buf.getForWriter()->value = 10;
  buf.publish();
  EXPECT_EQ(buf.getForReader()->value, 10);

  buf.getForWriter()->value = 20;
  buf.publish();
  EXPECT_EQ(buf.getForReader()->value, 20);

  buf.getForWriter()->value = 30;
  buf.publish();
  EXPECT_EQ(buf.getForReader()->value, 30);
}

TEST_F(TripleBufferTest, ReaderPointerAfterReadIsDistinctFromWriter) {
  TripleBuffer<IntVal> buf;

  buf.getForWriter()->value = 1;
  buf.publish();

  auto *reader = buf.getForReader();
  auto *writer = buf.getForWriter();
  EXPECT_NE(reader, writer);
}

TEST_F(TripleBufferTest, AllThreeBufferAddressesAreDistinct) {
  TripleBuffer<IntVal> buf;

  // Initial state: front=0, idle=1, back=2
  auto *front = buf.getForReader(); // buffers_[0]
  auto *back = buf.getForWriter();  // buffers_[2]

  // publish: back (2) goes to state, old idle (1) becomes new back
  buf.getForWriter()->value = 1;
  buf.publish();

  // consume the update: reader gets old back (2) as new front
  buf.getForReader();

  // after consume: front=2, state=0, back=1 — writer is now on the idle buffer
  auto *idle = buf.getForWriter(); // buffers_[1]

  EXPECT_NE(front, back);
  EXPECT_NE(front, idle);
  EXPECT_NE(back, idle);
}

// ---------------------------------------------------------------------------
// Stress Tests
// ---------------------------------------------------------------------------

TEST_F(TripleBufferTest, StressReaderValueNeverGoesBackward) {
  // Writer publishes strictly increasing values; reader must never see a
  // decrease (the triple buffer may skip values but never go backwards).
  TripleBuffer<IntVal> buf;
  std::atomic<bool> stop{false};
  std::atomic<bool> error{false};

  std::thread writer([&] {
    for (int i = 1; !stop.load(std::memory_order_relaxed); ++i) {
      buf.getForWriter()->value = i;
      buf.publish();
    }
  });

  std::thread reader([&] {
    int lastSeen = 0;
    while (!stop.load(std::memory_order_relaxed)) {
      int val = buf.getForReader()->value;
      if (val < lastSeen) {
        error.store(true, std::memory_order_relaxed);
      }
      lastSeen = val;
    }
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  stop.store(true);
  writer.join();
  reader.join();

  EXPECT_FALSE(error.load());
}

TEST_F(TripleBufferTest, StressWriterNeverBlocks) {
  TripleBuffer<IntVal> buf;

  constexpr int N = 1'000'000;
  for (int i = 0; i < N; ++i) {
    buf.getForWriter()->value = i;
    buf.publish();
  }

  SUCCEED();
}

TEST_F(TripleBufferTest, StressReaderNeverBlocks) {
  TripleBuffer<IntVal> buf;

  constexpr int N = 1'000'000;
  for (int i = 0; i < N; ++i) {
    (void)buf.getForReader()->value;
  }

  SUCCEED();
}

TEST_F(TripleBufferTest, StressBothSidesMakeProgress) {
  // Verify that neither thread starves the other.
  TripleBuffer<IntVal> buf;
  std::atomic<bool> stop{false};
  std::atomic<int> writeCount{0};
  std::atomic<int> readCount{0};

  std::thread writer([&] {
    for (int i = 1; !stop.load(std::memory_order_relaxed); ++i) {
      buf.getForWriter()->value = i;
      buf.publish();
      writeCount.fetch_add(1, std::memory_order_relaxed);
    }
  });

  std::thread reader([&] {
    while (!stop.load(std::memory_order_relaxed)) {
      (void)buf.getForReader()->value;
      readCount.fetch_add(1, std::memory_order_relaxed);
    }
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  stop.store(true);
  writer.join();
  reader.join();

  EXPECT_GT(writeCount.load(), 1'000);
  EXPECT_GT(readCount.load(), 1'000);
}

// NOLINTEND
