import AudioNode from './AudioNode';
import type AudioContext from './AudioContext';
import { IAudioContext } from '../jsi-interfaces';
import type { AudioTagHandle, InternalAudioTagHandle } from '../Audio/types';
import { InvalidStateError } from '../errors';

export interface MediaElementAudioSourceOptions {
  mediaElement: AudioTagHandle;
}

/**
 * Routes an `<Audio>` tag into the audio graph. From the moment this node is
 * created the tag is heard only through it: an unconnected or disconnected node
 * leaves the tag silent while it keeps playing, as the Web Audio API specifies.
 * A tag can be taken over by one node only.
 */
export default class MediaElementAudioSourceNode extends AudioNode {
  readonly mediaElement: AudioTagHandle;

  constructor(context: AudioContext, options: MediaElementAudioSourceOptions) {
    const audioTag = options.mediaElement as InternalAudioTagHandle;
    if (audioTag.getMediaElementSource() !== null) {
      throw new InvalidStateError(
        'Audio tag source is already routed through MediaElementAudioSourceNode.'
      );
    }

    super(
      context,
      (context.context as IAudioContext).createMediaElementSource()
    );
    this.mediaElement = options.mediaElement;
    audioTag.setMediaElementSource(this);
  }
}
