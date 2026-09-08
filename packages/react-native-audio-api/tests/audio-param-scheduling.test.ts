/* eslint-disable */

import AudioParam from '../src/core/AudioParam';
import { RangeError } from '../src/errors';
import type { IAudioParam } from '../src/jsi-interfaces';
import type BaseAudioContext from '../src/core/BaseAudioContext';

const createNativeParam = () =>
  ({
    value: 1,
    defaultValue: 1,
    minValue: -3.4028235e38,
    maxValue: 3.4028235e38,
    setValueAtTime: jest.fn(),
    linearRampToValueAtTime: jest.fn(),
    exponentialRampToValueAtTime: jest.fn(),
    setTargetAtTime: jest.fn(),
    setValueCurveAtTime: jest.fn(),
    cancelScheduledValues: jest.fn(),
    cancelAndHoldAtTime: jest.fn(),
    checkCurveExclusion: jest.fn(() => ({ status: 'success' })),
  }) as unknown as IAudioParam;

describe('AudioParam scheduling', () => {
  let nativeParam: IAudioParam;
  let param: AudioParam;

  beforeEach(() => {
    nativeParam = createNativeParam();
    // currentTime is 0 before an OfflineAudioContext starts rendering.
    const context = { currentTime: 0 } as unknown as BaseAudioContext;
    param = new AudioParam(nativeParam, context);
    jest.clearAllMocks();
  });

  describe('exponentialRampToValueAtTime', () => {
    it('accepts an endTime of 0', () => {
      expect(() => param.exponentialRampToValueAtTime(0.5, 0)).not.toThrow();
      expect(nativeParam.exponentialRampToValueAtTime).toHaveBeenCalledWith(
        0.5,
        0
      );
    });

    it('throws RangeError for a negative endTime', () => {
      expect(() => param.exponentialRampToValueAtTime(0.5, -1)).toThrow(
        RangeError
      );
      expect(nativeParam.exponentialRampToValueAtTime).not.toHaveBeenCalled();
    });

    it('throws RangeError for a target value of 0', () => {
      expect(() => param.exponentialRampToValueAtTime(0, 1)).toThrow(
        RangeError
      );
      expect(nativeParam.exponentialRampToValueAtTime).not.toHaveBeenCalled();
    });
  });

  describe('endTime of 0 is accepted by every automation method', () => {
    it('setValueAtTime', () => {
      expect(() => param.setValueAtTime(0.5, 0)).not.toThrow();
      expect(nativeParam.setValueAtTime).toHaveBeenCalledWith(0.5, 0);
    });

    it('linearRampToValueAtTime', () => {
      expect(() => param.linearRampToValueAtTime(0.5, 0)).not.toThrow();
      expect(nativeParam.linearRampToValueAtTime).toHaveBeenCalledWith(0.5, 0);
    });

    it('setTargetAtTime', () => {
      expect(() => param.setTargetAtTime(0.5, 0, 1)).not.toThrow();
      expect(nativeParam.setTargetAtTime).toHaveBeenCalledWith(0.5, 0, 1);
    });

    it('setValueCurveAtTime', () => {
      const curve = new Float32Array([0, 1]);
      expect(() => param.setValueCurveAtTime(curve, 0, 1)).not.toThrow();
      expect(nativeParam.setValueCurveAtTime).toHaveBeenCalledWith(curve, 0, 1);
    });
  });
});
