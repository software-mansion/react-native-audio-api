/* eslint-disable */

import * as MockAPI from 'react-native-audio-api/mock';

describe('ChannelMergerNode / ChannelSplitterNode', () => {
  let context: MockAPI.AudioContext;

  beforeEach(() => {
    context = new MockAPI.AudioContext();
  });

  describe('ChannelMergerNode', () => {
    it('defaults to 6 inputs and a single mono-per-input output', () => {
      const merger = context.createChannelMerger();
      expect(merger).toBeInstanceOf(MockAPI.ChannelMergerNode);
      expect(merger.numberOfInputs).toBe(6);
      expect(merger.numberOfOutputs).toBe(1);
      expect(merger.channelCount).toBe(1);
      expect(merger.channelCountMode).toBe('explicit');
    });

    it('honours a custom numberOfInputs', () => {
      const merger = context.createChannelMerger(2);
      expect(merger.numberOfInputs).toBe(2);
    });

    it('accepts connections into any valid input index', () => {
      const merger = context.createChannelMerger(4);
      const osc = context.createOscillator();
      expect(() => osc.connect(merger, 0, 3)).not.toThrow();
    });

    it('throws IndexSizeError for an out-of-range input index', () => {
      const merger = context.createChannelMerger(4);
      const osc = context.createOscillator();
      expect(() => osc.connect(merger, 0, 4)).toThrow(MockAPI.IndexSizeError);
    });
  });

  describe('ChannelSplitterNode', () => {
    it('defaults to 6 outputs with a discrete N-channel input', () => {
      const splitter = context.createChannelSplitter();
      expect(splitter).toBeInstanceOf(MockAPI.ChannelSplitterNode);
      expect(splitter.numberOfInputs).toBe(1);
      expect(splitter.numberOfOutputs).toBe(6);
      expect(splitter.channelCount).toBe(6);
      expect(splitter.channelCountMode).toBe('explicit');
      expect(splitter.channelInterpretation).toBe('discrete');
    });

    it('honours a custom numberOfOutputs', () => {
      const splitter = context.createChannelSplitter(3);
      expect(splitter.numberOfOutputs).toBe(3);
      expect(splitter.channelCount).toBe(3);
    });

    it('connects from any valid output index', () => {
      const splitter = context.createChannelSplitter(4);
      const gain = context.createGain();
      expect(() => splitter.connect(gain, 3)).not.toThrow();
    });

    it('throws IndexSizeError for an out-of-range output index', () => {
      const splitter = context.createChannelSplitter(4);
      const gain = context.createGain();
      expect(() => splitter.connect(gain, 4)).toThrow(MockAPI.IndexSizeError);
    });
  });

  describe('indexed connect / disconnect', () => {
    it('routes a splitter output into a merger input', () => {
      const splitter = context.createChannelSplitter(2);
      const merger = context.createChannelMerger(2);

      const result = splitter.connect(merger, 1, 0);
      expect(result).toBe(merger);
    });

    it('supports disconnecting a specific output index', () => {
      const splitter = context.createChannelSplitter(2);
      const gain = context.createGain();
      splitter.connect(gain, 1);
      expect(() => splitter.disconnect(1)).not.toThrow();
    });

    it('throws IndexSizeError when disconnecting an invalid output index', () => {
      const splitter = context.createChannelSplitter(2);
      expect(() => splitter.disconnect(5)).toThrow(MockAPI.IndexSizeError);
    });
  });
});
