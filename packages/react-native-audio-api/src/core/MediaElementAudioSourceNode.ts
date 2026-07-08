import AudioNode from './AudioNode';
import AudioParam from './AudioParam';
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
    fileSourceNode.disconnect(undefined);
    super(context, node);
    this.mediaElement = options.mediaElement;
  }

  public override disconnect(destination?: AudioNode | AudioParam): void {
    super.disconnect(destination);

    const fileSourceNode = (
      this.mediaElement as InternalAudioTagHandle
    ).getFileSourceNode();
    if (fileSourceNode !== null && !fileSourceNode.routedThroughMediaElement) {
      fileSourceNode.connect(this.context.context.destination);
    }
  }
}
