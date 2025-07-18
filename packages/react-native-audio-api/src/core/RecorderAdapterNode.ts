import { IRecorderAdapterNode } from '../interfaces';
import AudioNode from './AudioNode';
import AudioRecorder from './AudioRecorder';

export default class RecorderAdapterNode extends AudioNode {
  setRecorder(recorder: AudioRecorder): void {
    return (this.node as IRecorderAdapterNode).setRecorder(recorder.recorder);
  }
}
