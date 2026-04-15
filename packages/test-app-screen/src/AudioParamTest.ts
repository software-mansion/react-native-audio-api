import { OfflineAudioContext } from 'react-native-audio-api';
import type { AudioParam } from 'react-native-audio-api';
import { NotSupportedError } from '../../../packages/react-native-audio-api/src/errors';

const SAMPLE_RATE = 44100;
const TOLERANCE = 5e-3;

type SetInfo = (msg: string) => void;

function sampleAt(data: Float32Array, time: number): number {
  return data[Math.min(Math.round(time * SAMPLE_RATE), data.length - 1)];
}

function check(
  actual: number,
  expected: number,
  label: string,
  tol = TOLERANCE
): void {
  if (Math.abs(actual - expected) > tol) {
    throw new Error(
      `${label}: expected ≈${expected.toFixed(5)}, got ${actual.toFixed(5)} (diff=${Math.abs(actual - expected).toFixed(5)})`
    );
  }
}

async function renderGain(
  duration: number,
  setup: (gain: AudioParam) => void
): Promise<Float32Array> {
  const ctx = new OfflineAudioContext(
    1,
    Math.ceil(duration * SAMPLE_RATE),
    SAMPLE_RATE
  );
  const src = ctx.createConstantSource();
  const gainNode = ctx.createGain();
  src.connect(gainNode);
  gainNode.connect(ctx.destination);
  setup(gainNode.gain);
  src.start();
  const buffer = await ctx.startRendering();
  return buffer.getChannelData(0);
}

// --- setValueAtTime ---

export const audioParamSetValueAtTimeTest = async (
  setInfo: SetInfo
): Promise<void> => {
  setInfo('AudioParam: setValueAtTime');
  const data = await renderGain(0.4, (gain) => {
    gain.setValueAtTime(0.5, 0.0);
    gain.setValueAtTime(0.8, 0.1);
    gain.setValueAtTime(0.3, 0.2);
  });

  check(sampleAt(data, 0.05), 0.5, 'before first event');
  check(sampleAt(data, 0.12), 0.8, 'after first event');
  check(sampleAt(data, 0.15), 0.8, 'mid second interval');
  check(sampleAt(data, 0.22), 0.3, 'after second event');
  check(sampleAt(data, 0.35), 0.3, 'holds last value');
};

// --- linearRampToValueAtTime ---

export const audioParamLinearRampTest = async (
  setInfo: SetInfo
): Promise<void> => {
  setInfo('AudioParam: linearRampToValueAtTime');
  // v(t) = 0 + (1-0) * (t-0)/(0.2-0) = t/0.2
  const data = await renderGain(0.4, (gain) => {
    gain.setValueAtTime(0.0, 0.0);
    gain.linearRampToValueAtTime(1.0, 0.2);
  });

  check(sampleAt(data, 0.05), 0.25, '25% of ramp');
  check(sampleAt(data, 0.1), 0.5, '50% of ramp');
  check(sampleAt(data, 0.15), 0.75, '75% of ramp');
  check(sampleAt(data, 0.22), 1, 'past end holds at 1');
};

// --- exponentialRampToValueAtTime ---

export const audioParamExponentialRampTest = async (
  setInfo: SetInfo
): Promise<void> => {
  setInfo('AudioParam: exponentialRampToValueAtTime');
  // v(t) = 0.1 * (1.0/0.1)^(t/0.2) = 0.1 * 10^(t/0.2)
  const data = await renderGain(0.4, (gain) => {
    gain.setValueAtTime(0.1, 0.0);
    gain.exponentialRampToValueAtTime(1.0, 0.2);
  });

  check(sampleAt(data, 0.05), 0.1 * Math.pow(10, 0.25), 'exp 25%');
  check(sampleAt(data, 0.1), 0.1 * Math.pow(10, 0.5), 'exp 50%');
  check(sampleAt(data, 0.15), 0.1 * Math.pow(10, 0.75), 'exp 75%');
  check(sampleAt(data, 0.22), 1, 'exp past end holds at 1');
};

// --- setTargetAtTime ---

export const audioParamSetTargetAtTimeTest = async (
  setInfo: SetInfo
): Promise<void> => {
  setInfo('AudioParam: setTargetAtTime');
  // v(t) = 1 + (0 - 1) * exp(-(t - 0.1) / 0.1)  for t >= 0.1
  const data = await renderGain(0.6, (gain) => {
    gain.setValueAtTime(0.0, 0.0);
    gain.setTargetAtTime(1.0, 0.1, 0.1);
  });

  check(sampleAt(data, 0.05), 0, 'before setTarget start');
  // At exactly t=0.1: v = 1 + (0-1)*exp(0) = 0 (start value unchanged)
  check(sampleAt(data, 0.1), 0, 'at setTarget start');
  check(sampleAt(data, 0.15), 1 - Math.exp(-0.5), 'after 0.5 tau');
  check(sampleAt(data, 0.2), 1 - Math.exp(-1), 'after 1 tau');
  check(sampleAt(data, 0.3), 1 - Math.exp(-2), 'after 2 tau');
};

// --- setValueCurveAtTime ---

export const audioParamSetValueCurveAtTimeTest = async (
  setInfo: SetInfo
): Promise<void> => {
  setInfo('AudioParam: setValueCurveAtTime');
  // curve: [0.1, 0.4, 0.2, 0.8, 0.5] over 0.2s starting at t=0.1
  // k = (n-1)/duration * (t - startTime); lerp(curve[floor(k)], curve[ceil(k)], frac)
  const curve = new Float32Array([0.1, 0.4, 0.2, 0.8, 0.5]);
  const data = await renderGain(0.5, (gain) => {
    gain.setValueAtTime(0.0, 0.0);
    gain.setValueCurveAtTime(curve, 0.1, 0.2);
  });

  check(sampleAt(data, 0.05), 0.0, 'before curve');
  // k = 4/0.2 * (0.14 - 0.1) = 0.8 → lerp(0.1, 0.4, 0.8) = 0.1 + 0.3*0.8 = 0.34
  check(sampleAt(data, 0.14), 0.34, 'k=0.8 interpolated');
  // k = 4/0.2 * (0.18 - 0.1) = 1.6 → lerp(0.4, 0.2, 0.6) = 0.4 - 0.12 = 0.28
  check(sampleAt(data, 0.18), 0.28, 'k=1.6 interpolated');
  // After curve end (t=0.3): holds last value = 0.5
  check(sampleAt(data, 0.35), 0.5, 'past curve end holds last value');
};

// --- cancelScheduledValues ---

export const audioParamCancelScheduledValuesTest = async (
  setInfo: SetInfo
): Promise<void> => {
  setInfo('AudioParam: cancelScheduledValues');
  // Cancel at 0.15 removes events at t >= 0.15
  const data = await renderGain(0.5, (gain) => {
    gain.setValueAtTime(0, 0);
    gain.setValueAtTime(0.8, 0.1);
    gain.setValueAtTime(0.3, 0.2); // removed
    gain.linearRampToValueAtTime(1, 0.4); // removed
    gain.cancelScheduledValues(0.15);
  });

  check(sampleAt(data, 0.05), 0, 'initial value');
  check(sampleAt(data, 0.12), 0.8, 'first event survived');
  check(sampleAt(data, 0.22), 0.8, 'cancelled setValueAtTime gone');
  check(sampleAt(data, 0.35), 0.8, 'cancelled ramp gone, holds at 0.8');
};

// --- cancelAndHoldAtTime ---

export const audioParamCancelAndHoldAtTimeTest = async (
  setInfo: SetInfo
): Promise<void> => {
  setInfo('AudioParam: cancelAndHoldAtTime');
  // holdValue at t=0.15: 0.8 + (1.0 - 0.8) * (0.15-0.1)/(0.2-0.1) = 0.9
  const data = await renderGain(0.4, (gain) => {
    gain.setValueAtTime(0, 0);
    gain.setValueAtTime(0.8, 0.1);
    gain.linearRampToValueAtTime(1, 0.2);
    gain.cancelAndHoldAtTime(0.15);
  });

  check(sampleAt(data, 0.05), 0, 'initial value');
  check(sampleAt(data, 0.155), 0.9, 'hold value at cancel time');
  check(sampleAt(data, 0.25), 0.9, 'held after cancel time');
};

export const audioParamNotSupportedErrorTest = async (
  setInfo: SetInfo
): Promise<void> => {
  setInfo('AudioParam: NotSupportedError for unsupported methods');
  const ctx = await new OfflineAudioContext(1, SAMPLE_RATE, SAMPLE_RATE);
  const gain = ctx.createGain().gain;

  // Scheduling setValueCurveAtTime onto another event

  // Schedule setValueCurveAtTime onto a `startTime` point event
  try {
    gain.setValueAtTime(0.5, 0);
    gain.setValueCurveAtTime(new Float32Array([0, 1]), 0, 1);
  } catch (error) {
    if (error instanceof NotSupportedError) {
      setInfo(
        'scheduling setValueCurveAtTime correctly threw NotSupportedError'
      );
    } else {
      throw new Error(
        `A) Expected NotSupportedError, but got ${error instanceof Error ? error.name : String(error)}`
      );
    }
  }

  // Schedule setValueCurveAtTime onto a `endTime` point event
  try {
    gain.exponentialRampToValueAtTime(1, 0.5);
    gain.setValueCurveAtTime(new Float32Array([0, 1]), 0, 1);
  } catch (error) {
    if (error instanceof NotSupportedError) {
      setInfo(
        'scheduling setValueCurveAtTime correctly threw NotSupportedError'
      );
    } else {
      throw new Error(
        `B) Expected NotSupportedError, but got ${error instanceof Error ? error.name : String(error)}`
      );
    }
  }

  // Schedule setValueCurveAtTime onto another setValueCurveAtTime event
  try {
    gain.setValueCurveAtTime(new Float32Array([0, 1]), 0, 0.5);
    gain.setValueCurveAtTime(new Float32Array([0, 1]), 0, 1);
  } catch (error) {
    if (error instanceof NotSupportedError) {
      setInfo(
        'scheduling setValueCurveAtTime correctly threw NotSupportedError'
      );
    } else {
      throw new Error(
        `C) Expected NotSupportedError, but got ${error instanceof Error ? error.name : String(error)}`
      );
    }
  }

  // Scheduling point events onto a setValueCurveAtTime event

  // Schedule `startTime` onto a setValueCurveAtTime event
  try {
    gain.setValueCurveAtTime(new Float32Array([0, 1]), 0, 1);
    gain.setValueAtTime(0.5, 0);
  } catch (error) {
    if (error instanceof NotSupportedError) {
      setInfo(
        'scheduling setValueAtTime onto setValueCurveAtTime correctly threw NotSupportedError'
      );
    } else {
      throw new Error(
        `D) Expected NotSupportedError, but got ${error instanceof Error ? error.name : String(error)}`
      );
    }
  }

  // Schedule `endTime` onto a setValueCurveAtTime event
  try {
    gain.setValueCurveAtTime(new Float32Array([0, 1]), 0, 1);
    gain.exponentialRampToValueAtTime(1, 0.5);
  } catch (error) {
    if (error instanceof NotSupportedError) {
      setInfo(
        'scheduling exponentialRampToValueAtTime onto setValueCurveAtTime correctly threw NotSupportedError'
      );
    } else {
      throw new Error(
        `E) Expected NotSupportedError, but got ${error instanceof Error ? error.name : String(error)}`
      );
    }
  }
};

export const audioParamTestSuite = async (setInfo: SetInfo): Promise<void> => {
  await audioParamSetValueAtTimeTest(setInfo);
  await audioParamLinearRampTest(setInfo);
  await audioParamExponentialRampTest(setInfo);
  await audioParamSetTargetAtTimeTest(setInfo);
  await audioParamSetValueCurveAtTimeTest(setInfo);
  await audioParamCancelScheduledValuesTest(setInfo);
  await audioParamCancelAndHoldAtTimeTest(setInfo);
  await audioParamNotSupportedErrorTest(setInfo);
};
