import AudioNode from './AudioNode.web';
import type BaseAudioContext from './BaseAudioContext.web';

export interface MediaElementAudioSourceOptions {
  mediaElement: HTMLMediaElement;
}

export default class MediaElementAudioSourceNode extends AudioNode {
  constructor(
    context: BaseAudioContext,
    node: globalThis.MediaElementAudioSourceNode,
    readonly mediaElement: HTMLMediaElement
  ) {
    super(context, node);
  }
}
