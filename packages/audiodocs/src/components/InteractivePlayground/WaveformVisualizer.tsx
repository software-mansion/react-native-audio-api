import React, { FC, useCallback, useEffect, useRef } from "react";

import ResponsiveCanvas, {
  ResponsiveCanvasDrawParams,
} from "@site/src/ui/ResponsiveCanvas";
import { AnalyserNode } from "react-native-audio-api";

export const WaveformVisualizer: FC<{
  analyserNode: AnalyserNode | null;
  fftSize: number;
  theme: string;
}> = ({ analyserNode, fftSize, theme }) => {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const animationFrameRef = useRef<number | null>(null);

  const drawWaveform = useCallback(
    (canvas: HTMLCanvasElement) => {
      const context = canvas.getContext("2d");
      if (!context) {
        return;
      }

      const cssWidth = canvas.clientWidth || 300;
      const cssHeight = canvas.clientHeight || 150;
      const dpr =
        cssWidth > 0 && canvas.width > 0
          ? canvas.width / cssWidth
          : window.devicePixelRatio || 1;

      context.setTransform(dpr, 0, 0, dpr, 0, 0);
      context.clearRect(0, 0, cssWidth, cssHeight);

      context.lineWidth = 2;
      context.strokeStyle = theme === "dark" ? "#ff7774" : "#fa7f7c";
      context.beginPath();

      const data = new Uint8Array(fftSize);

      if (analyserNode) {
        analyserNode.getByteTimeDomainData(data);
      } else {
        data.fill(128); // Default to a flat line if no analyser node is available
      }

      const sliceWidth = cssWidth / (data.length - 1);

      let x = 0;

      for (let i = 0; i < data.length; i += 1) {
        const v = data[i] / 128.0;
        const y = (v * cssHeight) / 2;

        if (i === 0) {
          context.moveTo(x, y);
        } else {
          context.lineTo(x, y);
        }
        x += sliceWidth;
      }

      context.stroke();
    },
    [analyserNode, fftSize, theme]
  );

  const handleCanvasDraw = useCallback(
    ({ canvas }: ResponsiveCanvasDrawParams) => {
      drawWaveform(canvas);
    },
    [drawWaveform]
  );

  useEffect(() => {
    const frame = () => {
      const canvas = canvasRef.current;
      if (canvas) {
        drawWaveform(canvas);
      }
      animationFrameRef.current = requestAnimationFrame(frame);
    };

    animationFrameRef.current = requestAnimationFrame(frame);

    return () => {
      if (animationFrameRef.current) {
        cancelAnimationFrame(animationFrameRef.current);
      }
    };
  }, [drawWaveform]);

  return (
    <ResponsiveCanvas
      onDraw={handleCanvasDraw}
      canvasRef={canvasRef}
      throttleMs={16}
    />
  );
};
