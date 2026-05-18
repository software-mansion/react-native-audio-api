import AudioNode from './AudioNode';
import type BaseAudioContext from './BaseAudioContext';

export interface MediaElementAudioSourceOptions {
  mediaElement: HTMLMediaElement;
}

export default class MediaElementAudioSourceNode extends AudioNode {
  readonly mediaElement: HTMLMediaElement;

  constructor(
    context: BaseAudioContext,
    node: globalThis.MediaElementAudioSourceNode,
    mediaElement: HTMLMediaElement
  ) {
    super(context, node);
    this.mediaElement = mediaElement;
  }
}
