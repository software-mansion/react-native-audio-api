import AudioNode from './AudioNode';
import type AudioContext from './AudioContext';
import { IAudioContext } from '../jsi-interfaces';
import type { AudioTagHandle, InternalAudioTagHandle } from '../Audio/types';
import { InvalidStateError } from '../errors';

export interface MediaElementAudioSourceOptions {
  mediaElement: AudioTagHandle;
}

export default class MediaElementAudioSourceNode extends AudioNode {
  readonly mediaElement: AudioTagHandle;

  constructor(context: AudioContext, options: MediaElementAudioSourceOptions) {
    const internalHandle = options.mediaElement as InternalAudioTagHandle;
    const fileSourceNode = internalHandle.getFileSourceNode();
    if (fileSourceNode === null) {
      throw new InvalidStateError(
        'Audio tag source is not ready yet. Wait for onLoad before creating MediaElementAudioSourceNode.'
      );
    }
    if (fileSourceNode.routedThroughMediaElement) {
      throw new InvalidStateError(
        'Audio tag source is already routed through MediaElementAudioSourceNode.'
      );
    }
    const node = (context.context as IAudioContext).createMediaElementSource(
      fileSourceNode
    );
    super(context, node);
    this.mediaElement = options.mediaElement;
  }
}
