import type { IAudioControls } from '../interfaces';
import {
  EventListener,
  NowPlayingInfo,
  AudioEventName,
  AudioEventCallback,
  AudioSessionOptions,
  NowPlayingInfoUpdate,
} from '../types';

const defaultOptions: AudioSessionOptions = {
  iosMode: 'default',
  iosCategory: 'playback',
  interruptionMode: 'automatic',
  iosCategoryOptions: ['mixWithOthers'],
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
