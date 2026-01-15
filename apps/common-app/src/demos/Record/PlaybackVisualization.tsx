import {
  Canvas,
  Group,
  Path,
  Skia,
  useCanvasRef,
  useCanvasSize,
} from '@shopify/react-native-skia';
import React, { useMemo } from 'react';
import { StyleSheet, View } from 'react-native';
import { AudioBuffer } from 'react-native-audio-api';
import { SharedValue, useDerivedValue } from 'react-native-reanimated';

interface PlaybackVisualizationProps {
  buffer: AudioBuffer | null;
  currentPositionSeconds: SharedValue<number>;
  durationSeconds: number;
}

const PlaybackVisualization: React.FC<PlaybackVisualizationProps> = (props) => {
  const { buffer, currentPositionSeconds, durationSeconds } = props;
  const canvasRef = useCanvasRef();
  const { size } = useCanvasSize(canvasRef);

  const bufferWaveform = useMemo(() => {
    const height = size.height;
    const width = size.width;

    if (!buffer || width === 0 || height === 0) {
      return [];
    }

    const channelData = buffer.getChannelData(0);
    const barWidth = 2;
    const barGap = 2;

    const totalBars = Math.floor(width / (barWidth + barGap));
    const samplesPerBar = Math.floor(channelData.length / totalBars);
    let maxValue = 0;

    const waveform = new Array(totalBars).fill(0);

    for (let i = 0; i < channelData.length; i++) {
      maxValue = Math.max(maxValue, Math.abs(channelData[i]));
    }

    for (let i = 0; i < totalBars; i++) {
      const startSample = i * samplesPerBar;
      const endSample = startSample + samplesPerBar;
      let max = 0;

      for (let j = startSample; j < endSample; j++) {
        max = Math.max(max, Math.abs(channelData[j]));
      }

      const barHeight = (max / maxValue) * height * 0.8;
      const x = i * (barWidth + barGap);
      const y = (height - barHeight) / 2;

      waveform[i] = { x, y, width: barWidth, height: barHeight };
    }

    return waveform;
  }, [buffer, size.height, size.width]);

  const bgPath = useMemo(() => {
    const path = Skia.Path.Make();

    bufferWaveform.forEach((bar) => {
      path.moveTo(bar.x, bar.y);
      path.lineTo(bar.x, bar.y + bar.height);
    });

    path.close();
    return path;
  }, [bufferWaveform]);

  const progressPath = useDerivedValue(() => {
    const path = Skia.Path.Make();

    const totalBars = bufferWaveform.length;
    const currentBar = Math.floor(
      (currentPositionSeconds.value / durationSeconds) * totalBars
    );

    for (let i = 0; i < currentBar; i++) {
      const bar = bufferWaveform[i];
      path.moveTo(bar.x, bar.y);
      path.lineTo(bar.x, bar.y + bar.height);
    }

    path.close();
    return path;
  }, [bufferWaveform, buffer]);

  if (!buffer) {
    return null;
  }

  return (
    <View style={styles.container}>
      <Canvas ref={canvasRef} style={styles.canvas}>
        <Group>
          <Path
            path={bgPath}
            color="rgba(255, 255, 255, 0.2)"
            style="stroke"
            strokeWidth={2}
            strokeCap="round"
          />
          <Path
            path={progressPath}
            color="#00a9f0"
            style="stroke"
            strokeWidth={2}
            strokeCap="round"
          />
        </Group>
      </Canvas>
    </View>
  );
};

export default PlaybackVisualization;

const styles = StyleSheet.create({
  container: {
    flex: 1,
    maxHeight: 250,
    width: '100%',
    backgroundColor: 'rgba(0, 0, 0, 0.15)',
    flexDirection: 'column',
  },
  canvas: {
    flex: 1,
  },
});
