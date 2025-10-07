This project contains source code copied from Webkit implementation, which mixes BSD 2 clause and BSD 3 clause with copyrights in specific files held by the following organizations:

- Google Inc.
- Apple Inc.

Individual files retain the original copyright notice, but not the full license text.

WebKit, 2 Clause BSD
Copyright (c) The WebKit Authors
[https://github.com/WebKit/webkit](https://github.com/WebKit/webkit)

WebKit, 3 Clause BSD
Copyright (c) The WebKit Authors
[https://github.com/WebKit/webkit](https://github.com/WebKit/webkit)

In addition we utilize FFmpeg library under GNU Lesser General Public License (LGPL) version 2.1 or later

- Source: https://github.com/FFmpeg/FFmpeg/releases/tag/n8.0
- Used in audio processing module [packages/react-native-audio-api/common/cpp/audioapi/core/sources/StreamerNode.cpp](https://github.com/software-mansion/react-native-audio-api/blob/main/packages/react-native-audio-api/common/cpp/audioapi/core/sources/StreamerNode.cpp)
- Used in audio decoding module [packages/react-native-audio-api/common/cpp/audioapi/libs/ffmpeg/FFmpegDecoding.cpp](https://github.com/software-mansion/react-native-audio-api/blob/main/packages/react-native-audio-api/common/cpp/audioapi/libs/ffmpeg/FFmpegDecoding.cpp)
- Instruction for relinking: [packages/react-native-audio-api/common/cpp/audioapi/libs/ffmpeg/INSTRUCTIONS.md](https://github.com/software-mansion/react-native-audio-api/blob/main/packages/react-native-audio-api/common/cpp/audioapi/libs/ffmpeg/INSTRUCTIONS.md)

Other LGPL components may be included as dependencies.
The full text of the LGPL license is included in lgplv3 file.