import type AudioParam from './AudioParam.web';
import type AudioBuffer from './AudioBuffer.web';
import type AudioNode from './AudioNode.web';

export interface AudioBufferSourceNodeBackend {
  readonly detune: AudioParam;
  readonly playbackRate: AudioParam;

  connect(destination: AudioNode | AudioParam): AudioNode | AudioParam;
  disconnect(destination?: AudioNode | AudioParam): void;

  start(when?: number, offset?: number, duration?: number): void;
  stop(when: number): void;

  get buffer(): AudioBuffer | null;
  set buffer(buffer: AudioBuffer | null);

  get loop(): boolean;
  set loop(value: boolean);

  get loopStart(): number;
  set loopStart(value: number);

  get loopEnd(): number;
  set loopEnd(value: number);

  get loopSkip(): boolean;
  set loopSkip(value: boolean);

  get onLoopEnded(): ((event: object) => void) | undefined;
  set onLoopEnded(callback: ((event: object) => void) | null);
}
