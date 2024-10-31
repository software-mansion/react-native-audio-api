import { IAudioDestinationNode } from '../interfaces';
import AudioNode from './AudioNode';
import BaseAudioContext from './BaseAudioContext';

export default class AudioDestinationNode extends AudioNode {
  constructor(context: BaseAudioContext, destination: IAudioDestinationNode) {
    super(context, destination);
  }
}
