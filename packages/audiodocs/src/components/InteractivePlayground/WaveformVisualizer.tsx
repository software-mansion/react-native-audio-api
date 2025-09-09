import React, { FC, useEffect, useRef } from "react";

import { AnalyserNode } from "react-native-audio-api";

export const WaveformVisualizer: FC<{
  analyserNode: AnalyserNode | null;
  fftSize: number;
  theme: string;
}> = ({ analyserNode, fftSize, theme }) => {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const animationFrameRef = useRef<number | null>(null);

  useEffect(() => {
    const draw = () => {
      const canvas = canvasRef.current;
      if (!canvas || !analyserNode) return;
      const context = canvas.getContext("2d");

      if (!context) return;

      const dpr = window.devicePixelRatio || 1;
      const logicalWidth = canvas.clientWidth || 300;
      const logicalHeight = canvas.clientHeight || 150;
      canvas.width = logicalWidth * dpr;
      canvas.height = logicalHeight * dpr;

      context.setTransform(dpr, 0, 0, dpr, 0, 0);
      context.clearRect(0, 0, logicalWidth, logicalHeight);
      context.lineWidth = 2;
      context.strokeStyle = theme === "dark" ? "#55b1e3" : "#38acdd";
      context.beginPath();

      const data = new Uint8Array(fftSize);
      analyserNode.getByteTimeDomainData(data);

      const sliceWidth = logicalWidth / data.length;

      let x = 0;
      for (let i = 0; i < data.length; i++) {
        const v = data[i] / 128.0;
        const y = (v * logicalHeight) / 2;

        if (i === 0) {
          context.moveTo(x, y);
        } else {
          context.lineTo(x, y);
        }
        x += sliceWidth;
      }

      context.lineTo(logicalWidth, logicalHeight / 2);
      context.stroke();
      animationFrameRef.current = requestAnimationFrame(draw);
    };
    animationFrameRef.current = requestAnimationFrame(draw);

    return () => {
      if (animationFrameRef.current) {
        cancelAnimationFrame(animationFrameRef.current);
      }
    };
  }, [analyserNode, fftSize, theme]);

  return <canvas ref={canvasRef} style={{ width: "100%", height: "100%" }} />;
};
