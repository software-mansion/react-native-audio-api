#pragma once

#include <jsi/jsi.h>
#include <memory>
#include <string>

namespace audioapi {
using namespace facebook;

class NowPlayingInfo {
 public:
  std::string title;
  std::string artist;
  std::string artworkUrl;
  int duration;

  // Update only
  int elapsedTime;
  bool isPlaying;

  static std::shared_ptr<NowPlayingInfo> fromJSIValue(const jsi::Value &jsiValue, jsi::Runtime &runtime) {
    auto jsiOptions = jsiValue.getObject(runtime);
    auto options = std::make_shared<NowPlayingInfo>();

    options->title = jsiOptions.getProperty(runtime, "title").getString(runtime).utf8(runtime);
    options->artist = jsiOptions.getProperty(runtime, "artist").getString(runtime).utf8(runtime);
    options->artworkUrl = jsiOptions.getProperty(runtime, "artwork").getString(runtime).utf8(runtime);
    options->duration = jsiOptions.getProperty(runtime, "duration").getNumber();

    return options;
  }

  jsi::Value toJSIObject(jsi::Runtime &runtime) {
    auto jsiObject = jsi::Object(runtime);

    jsiObject.setProperty(runtime, "title", jsi::String::createFromUtf8(runtime, title));
    jsiObject.setProperty(runtime, "artist", jsi::String::createFromUtf8(runtime, artist));
    jsiObject.setProperty(runtime, "artwork", jsi::String::createFromUtf8(runtime, artworkUrl));
    jsiObject.setProperty(runtime, "duration", jsi::Value(duration));

    return jsiObject;
  }
};

} // namespace audioapi
