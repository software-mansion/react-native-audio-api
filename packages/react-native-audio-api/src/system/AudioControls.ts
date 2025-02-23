import type { IAudioControls } from '../interfaces';
import {
  IOSMode,
  IOSCategory,
  EventListener,
  NowPlayingInfo,
  AudioEventName,
  InterruptionMode,
  IOSCategoryOption,
  AudioEventCallback,
  AudioSessionOptions,
  NowPlayingInfoUpdate,
} from '../types';

const defaultOptions: AudioSessionOptions = {
  iosCategory: IOSCategory.Playback,
  iosMode: IOSMode.Default,
  iosCategoryOptions: [
    IOSCategoryOption.DuckOthers,
    IOSCategoryOption.MixWithOthers,
  ],
  interruptionMode: InterruptionMode.Automatic,
  androidForegroundService: true,
};

function withDefaultOptions(
  options?: AudioSessionOptions
): AudioSessionOptions {
  return {
    ...defaultOptions,
    ...(options ?? {}),
  };
}

class AudioControls {
  protected module(): IAudioControls {
    return global.__AudioAPIInstaller.getAudioControls();
  }

  init(options?: AudioSessionOptions) {
    this.module().init(withDefaultOptions(options));
  }

  updateOptions(options?: AudioSessionOptions) {
    this.module().updateOptions(withDefaultOptions(options));
  }

  disable() {
    this.module().disable();
  }

  showNowPlayingInfo(info: NowPlayingInfo) {
    this.module().showNowPlayingInfo(info);
  }

  updateNowPlayingInfo(info: NowPlayingInfoUpdate) {
    this.module().updateNowPlayingInfo(info);
  }

  hideNowPlayingInfo() {
    this.module().hideNowPlayingInfo();
  }

  addEventListener<EN extends AudioEventName>(
    eventName: EN,
    eventHandler: AudioEventCallback<EN>
  ): EventListener {
    this.module().addEventListener(eventName, eventHandler);

    return {
      // TODO:
      remove: () => {},
    };
  }
}

export default new AudioControls();
