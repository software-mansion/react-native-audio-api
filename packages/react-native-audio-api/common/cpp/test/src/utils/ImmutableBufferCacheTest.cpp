#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/ImmutableBufferCache.hpp>
#include <gtest/gtest.h>
#include <memory>

using namespace audioapi;
using namespace audioapi::utils;

// NOLINTBEGIN

namespace {

constexpr size_t FRAME_COUNT = 1024;
constexpr int CHANNELS = 2;
constexpr float SAMPLE_RATE = 44100.0f;

std::shared_ptr<AudioBuffer> makeBuffer() {
  return std::make_shared<AudioBuffer>(FRAME_COUNT, CHANNELS, SAMPLE_RATE);
}

} // namespace

// The core of https://github.com/software-mansion/react-native-audio-api/issues/1263:
// repeated `.buffer = x` reassignment of the same underlying buffer (the seek pattern)
// must not allocate a fresh full-size copy every time.
TEST(ImmutableBufferCacheTest, ReusesCachedCopyAcrossRepeatedCalls) {
  ImmutableBufferCache cache;
  auto source = makeBuffer();

  auto first = cache.getOrCreate(source);
  auto second = cache.getOrCreate(source);
  auto third = cache.getOrCreate(source);

  EXPECT_EQ(first, second) << "Second call should reuse the cached copy, not allocate a new one.";
  EXPECT_EQ(second, third) << "Third call should reuse the same cached copy as well.";
}

TEST(ImmutableBufferCacheTest, CachedCopyIsAnActualCopyNotTheOriginal) {
  ImmutableBufferCache cache;
  auto source = makeBuffer();

  auto copy = cache.getOrCreate(source);

  EXPECT_NE(copy, source) << "The cached copy must be a distinct AudioBuffer instance. Sharing "
                             "the original directly would let a future mutation of it race with "
                             "a node concurrently reading the \"copy\".";
}

TEST(ImmutableBufferCacheTest, InvalidateForcesAFreshCopyOnce) {
  ImmutableBufferCache cache;
  auto source = makeBuffer();

  auto first = cache.getOrCreate(source);
  cache.invalidate();
  auto second = cache.getOrCreate(source);
  auto third = cache.getOrCreate(source);

  EXPECT_NE(first, second) << "invalidate() means the source was just mutated in place, so the "
                              "previously cached copy is stale and must not be reused.";
  EXPECT_EQ(second, third) << "After producing one fresh copy, subsequent calls should resume "
                              "caching normally rather than copying every time.";
}

TEST(ImmutableBufferCacheTest, MarkLiveViewEscapedDisablesCachingPermanently) {
  ImmutableBufferCache cache;
  auto source = makeBuffer();

  auto first = cache.getOrCreate(source);
  cache.markLiveViewEscaped();
  auto second = cache.getOrCreate(source);
  auto third = cache.getOrCreate(source);

  EXPECT_NE(first, second) << "A live, JS-writable view escaped, so the pre-existing cache entry "
                              "must be dropped since we can no longer prove it stayed in sync.";
  EXPECT_NE(second, third)
      << "Once a live view has ever escaped, every future call must fall back to a fresh, "
         "uncached copy indefinitely. A write through that view could happen at any later "
         "time, not just at the moment it was retrieved.";
}

// NOLINTEND
