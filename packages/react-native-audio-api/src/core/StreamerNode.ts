import AudioScheduledSourceNode from './AudioScheduledSourceNode';
import { StreamerOptions } from '../types';
import { NotSupportedError } from '../errors';
import type BaseAudioContext from './BaseAudioContext';

/**
 * StreamerNode is an AudioNode that allows you to stream audio from a file or
 * URL. It uses FFmpeg to decode the audio data and provides it as an audio
 * source. Note: This node requires the build to use FFmpeg, and will throw an
 * error if FFmpeg is not available.
 *
 * @deprecated Use the `<Audio>` element with MediaElementAudioSourceNode for
 *   streaming audio instead to achieve the same functionality.
 */
export default class StreamerNode extends AudioScheduledSourceNode {
  readonly streamPath: string;

  constructor(context: BaseAudioContext, options: StreamerOptions) {
    if (__DEV__) {
      console.warn(
        'StreamerNode is deprecated and will be removed in a future version. ' +
          'Please migrate to using the <Audio> element for streaming audio.'
      );
    }

    const node = context.context.createStreamer(options);
    if (!node) {
      throw new NotSupportedError('StreamerNode requires FFmpeg build');
    }
    super(context, node);
    this.streamPath = options.streamPath;
  }
}
