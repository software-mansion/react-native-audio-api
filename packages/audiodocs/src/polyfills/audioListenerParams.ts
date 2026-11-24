/**
 * Gives `AudioListener` the nine `AudioParam` attributes on browsers that only ship the deprecated
 * `setPosition` / `setOrientation` methods (Firefox).
 *
 * The `react-native-audio-api` web `AudioContext` wraps `listener.positionX` … `listener.upZ` in its
 * own `AudioParam` while it is constructed, so without these attributes every context in the docs
 * throws before a `PannerNode` — or anything else — can be created.
 *
 * The browser keeps no readable listener pose behind `setPosition` / `setOrientation`, so the facade
 * remembers the last values it wrote. The listener cannot be automated there either: every
 * scheduling method applies its final value immediately and the cancel methods do nothing.
 */

type Vector3 = [number, number, number];

interface ListenerPose {
  position: Vector3;
  forward: Vector3;
  up: Vector3;
}

type PoseVector = keyof ListenerPose;

const LISTENER_PARAMS = {
  positionX: { vector: 'position', axis: 0 },
  positionY: { vector: 'position', axis: 1 },
  positionZ: { vector: 'position', axis: 2 },
  forwardX: { vector: 'forward', axis: 0 },
  forwardY: { vector: 'forward', axis: 1 },
  forwardZ: { vector: 'forward', axis: 2 },
  upX: { vector: 'up', axis: 0 },
  upY: { vector: 'up', axis: 1 },
  upZ: { vector: 'up', axis: 2 },
} as const satisfies Record<string, { vector: PoseVector; axis: 0 | 1 | 2 }>;

type ListenerParamName = keyof typeof LISTENER_PARAMS;

const FLOAT32_MAX = 3.4028234663852886e38;

const posesByListener = new WeakMap<AudioListener, ListenerPose>();
const paramsByListener = new WeakMap<AudioListener, Map<ListenerParamName, AudioParam>>();

function defaultPose(): ListenerPose {
  return { position: [0, 0, 0], forward: [0, 0, -1], up: [0, 1, 0] };
}

function poseOf(listener: AudioListener): ListenerPose {
  let pose = posesByListener.get(listener);
  if (!pose) {
    pose = defaultPose();
    posesByListener.set(listener, pose);
  }
  return pose;
}

function writePose(listener: AudioListener, { position, forward, up }: ListenerPose): void {
  listener.setPosition(...position);
  listener.setOrientation(...forward, ...up);
}

class ImmediateListenerParam implements AudioParam {
  readonly automationRate: AutomationRate = 'a-rate';
  readonly defaultValue: number;
  readonly minValue = -FLOAT32_MAX;
  readonly maxValue = FLOAT32_MAX;

  constructor(
    private readonly listener: AudioListener,
    private readonly vector: PoseVector,
    private readonly axis: 0 | 1 | 2
  ) {
    this.defaultValue = defaultPose()[vector][axis];
  }

  get value(): number {
    return poseOf(this.listener)[this.vector][this.axis];
  }

  set value(value: number) {
    const pose = poseOf(this.listener);
    pose[this.vector][this.axis] = value;
    writePose(this.listener, pose);
  }

  setValueAtTime(value: number): AudioParam {
    this.value = value;
    return this;
  }

  linearRampToValueAtTime(value: number): AudioParam {
    return this.setValueAtTime(value);
  }

  exponentialRampToValueAtTime(value: number): AudioParam {
    return this.setValueAtTime(value);
  }

  setTargetAtTime(target: number): AudioParam {
    return this.setValueAtTime(target);
  }

  setValueCurveAtTime(values: Iterable<number>): AudioParam {
    const curve = Array.from(values);
    if (curve.length > 0) {
      this.value = curve[curve.length - 1];
    }
    return this;
  }

  cancelScheduledValues(): AudioParam {
    return this;
  }

  cancelAndHoldAtTime(): AudioParam {
    return this;
  }
}

function listenerParam(listener: AudioListener, name: ListenerParamName): AudioParam {
  let params = paramsByListener.get(listener);
  if (!params) {
    params = new Map();
    paramsByListener.set(listener, params);
  }

  let param = params.get(name);
  if (!param) {
    const { vector, axis } = LISTENER_PARAMS[name];
    param = new ImmediateListenerParam(listener, vector, axis);
    params.set(name, param);
  }
  return param;
}

function installAudioListenerParams(): void {
  if (typeof window === 'undefined' || typeof window.AudioListener === 'undefined') {
    return;
  }

  const listenerPrototype = window.AudioListener.prototype;
  if ('positionX' in listenerPrototype) {
    return;
  }

  for (const name of Object.keys(LISTENER_PARAMS) as ListenerParamName[]) {
    Object.defineProperty(listenerPrototype, name, {
      configurable: true,
      enumerable: true,
      get(this: AudioListener) {
        return listenerParam(this, name);
      },
    });
  }
}

installAudioListenerParams();
