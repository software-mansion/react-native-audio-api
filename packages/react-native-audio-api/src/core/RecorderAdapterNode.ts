import { IRecorderAdapterNode } from '../interfaces';
import AudioScheduledSourceNode from './AudioScheduledSourceNode';
import AudioRecorder from './AudioRecorder';

export default class RecorderAdapterNode extends AudioScheduledSourceNode {
  setRecorder(recorder: AudioRecorder): void {
    return (this.node as IRecorderAdapterNode).setRecorder(recorder.recorder);
  }
}
