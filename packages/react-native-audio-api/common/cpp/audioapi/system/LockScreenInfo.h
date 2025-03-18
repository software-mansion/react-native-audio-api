#pragma once

#include <jsi/jsi.h>
#include <string>
#include <memory>

namespace audioapi {

using namespace facebook;

class LockScreenInfo {
 public:
  static std::shared_ptr<LockScreenInfo> fromJSIValue(const jsi::Value &jsiValue, jsi::Runtime &runtime) {
    auto jsiLockScreenInfoObject = jsiValue.getObject(runtime);
    auto lockScreenInfo = std::make_shared<LockScreenInfo>();

    auto title = jsiLockScreenInfoObject.getProperty(runtime, "title");
    auto artwork = jsiLockScreenInfoObject.getProperty(runtime, "artwork");
    auto artist = jsiLockScreenInfoObject.getProperty(runtime, "artist");
    auto album = jsiLockScreenInfoObject.getProperty(runtime, "album");
    auto genre = jsiLockScreenInfoObject.getProperty(runtime, "genre");
    auto duration = jsiLockScreenInfoObject.getProperty(runtime, "duration");
    auto elapsedTime = jsiLockScreenInfoObject.getProperty(runtime, "elapsedTime");
    auto isLiveStream = jsiLockScreenInfoObject.getProperty(runtime, "isLiveStream");

    if (!title.isUndefined()) lockScreenInfo->title_ = title.getString(runtime).utf8(runtime);
    if (!artwork.isUndefined()) lockScreenInfo->artwork_ = artwork.getString(runtime).utf8(runtime);
    if (!artist.isUndefined()) lockScreenInfo->artist_ = artist.getString(runtime).utf8(runtime);
    if (!album.isUndefined()) lockScreenInfo->album_ = album.getString(runtime).utf8(runtime);
    if (!genre.isUndefined()) lockScreenInfo->genre_ = genre.getString(runtime).utf8(runtime);
    if (!duration.isUndefined()) lockScreenInfo->duration_ = duration.getNumber();
    if (!elapsedTime.isUndefined()) lockScreenInfo->elapsedTime_ = elapsedTime.getNumber();
    if (!isLiveStream.isUndefined()) lockScreenInfo->isLiveStream_ = isLiveStream.getBool();

    return lockScreenInfo;
  }

  std::string title_ {};
  std::string artwork_ {};
  std::string artist_ {};
  std::string album_ {};
  std::string genre_ {};
  double duration_ {-1};
  double elapsedTime_ {-1};
  bool isLiveStream_ {false};
};

} // namespace audioapi
