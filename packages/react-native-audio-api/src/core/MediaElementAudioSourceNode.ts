import AudioNode from './AudioNode';
import type AudioContext from './AudioContext';
import { IAudioContext } from '../interfaces';
import type {
  AudioTagHandle,
  InternalAudioTagHandle,
} from '../development/react/Audio/types';
import { NotSupportedError } from '../errors';

export interface MediaElementAudioSourceOptions {
  mediaElement: AudioTagHandle;
}

export default class MediaElementAudioSourceNode extends AudioNode {
  readonly mediaElement: AudioTagHandle;

  constructor(context: AudioContext, options: MediaElementAudioSourceOptions) {
    const internalHandle = options.mediaElement as InternalAudioTagHandle;
    const fileSourceNode = internalHandle.getMediaElementSourceNode();
    if (fileSourceNode === null) {
      throw new NotSupportedError(
        'Audio tag source is not ready yet. Wait for onLoad before creating MediaElementAudioSourceNode.'
      );
    }
    const node = (context.context as IAudioContext).createMediaElementSource(
      fileSourceNode
    );
    super(context, node);
    this.mediaElement = options.mediaElement;
  }
}
