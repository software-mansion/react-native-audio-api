#include <audioapi/utils/Path.h>
#include <gtest/gtest.h>
#include <string>

using namespace audioapi;

TEST(PathTest, NormalizesFileUrls) {
  EXPECT_EQ(path::normalizeFilePath("file:///tmp/audio%20segment.m4a"), "/tmp/audio segment.m4a");
}

TEST(PathTest, KeepsFilesystemPaths) {
  EXPECT_EQ(path::normalizeFilePath("/tmp/audio%20segment.m4a"), "/tmp/audio segment.m4a");
}

TEST(PathTest, FileUrlsRoundTripToTheSamePath) {
  EXPECT_EQ(path::toFileUrl("/tmp/take%20one.wav"), "file:///tmp/take%2520one.wav");
  EXPECT_EQ(path::toFileUrl("/tmp/take one.wav"), "file:///tmp/take%20one.wav");
  for (const std::string original :
       {"/tmp/take%20one.wav", "/tmp/take one.wav", "/tmp/AudioAPI/rec_1.m4a"}) {
    EXPECT_EQ(path::normalizeFilePath(path::toFileUrl(original)), original);
  }
}

TEST(PathTest, KeepsOrdinaryPathsReadableInFileUrls) {
  EXPECT_EQ(path::toFileUrl("/tmp/AudioAPI/rec_1.m4a"), "file:///tmp/AudioAPI/rec_1.m4a");
}

TEST(PathTest, KeepsMalformedPercentEscapes) {
  EXPECT_EQ(path::percentDecode("/tmp/100%.wav"), "/tmp/100%.wav");
  EXPECT_EQ(path::percentDecode("/tmp/%zz.wav"), "/tmp/%zz.wav");
}

TEST(PathTest, LowercasesTheExtension) {
  EXPECT_EQ(path::lowercaseExtension("/tmp/Take.WAV"), "wav");
}

TEST(PathTest, ReadsTheExtensionFromTheFileNameOnly) {
  EXPECT_EQ(path::lowercaseExtension("/tmp/session.v2/take"), "");
  EXPECT_EQ(path::lowercaseExtension("/tmp/take"), "");
}

TEST(PathTest, MatchesAnyListedExtension) {
  EXPECT_TRUE(path::hasExtension("/tmp/take.MP4", {"m4a", "mp4"}));
  EXPECT_FALSE(path::hasExtension("/tmp/take.flac", {"m4a", "mp4"}));
}

TEST(PathTest, DetectsNonFileProtocols) {
  EXPECT_TRUE(path::hasNonFileProtocol("https://example.com/take.wav"));
  EXPECT_TRUE(path::hasNonFileProtocol("content://media/take"));
  EXPECT_FALSE(path::hasNonFileProtocol("/tmp/take.wav"));
  EXPECT_FALSE(path::hasNonFileProtocol("/tmp/a:b.wav"));
}
