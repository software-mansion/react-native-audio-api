# FFmpeg additional information

We use [`ffmpeg`](https://github.com/FFmpeg/FFmpeg) for few components:

* [`StreamerNode`](/docs/sources/streamer-node)
* decoding `aac`, `mp4`, `m4a` files

## Disabling FFmpeg

The ffmpeg usage is enabled by default, however if you would like not to use it, f.e. there are some name clashes with other ffmpeg
binaries in your project, you can easily disable it. Just add one flag in corresponding file.

> **Info**
>
> FFmpeg is enabled by default

Add entry in [expo plugin](/docs/fundamentals/getting-started#step-2-add-audio-api-expo-plugin-optional).

```
"disableFFmpeg": true
```

Podfile

```
ENV['DISABLE_AUDIOAPI_FFMPEG'] = '1'
```

gradle.properties

```
disableAudioapiFFmpeg=true
```
