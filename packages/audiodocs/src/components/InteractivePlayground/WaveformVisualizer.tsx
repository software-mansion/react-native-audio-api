import React, { FC, useEffect, useRef } from "react";

export const WaveformVisualizer: FC<{ data: Uint8Array; theme: string }> = ({
  data,
  theme,
}) => {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const context = canvas.getContext("2d");
    if (!context) return;

    const { width, height } = canvas.getBoundingClientRect();
    canvas.width = width;
    canvas.height = height;

    context.clearRect(0, 0, width, height);
    context.lineWidth = 2;
    context.strokeStyle = theme === "dark" ? "#55b1e3" : "#38acdd";
    context.beginPath();

    const sliceWidth = (width * 1.0) / data.length;
    let x = 0;

    for (let i = 0; i < data.length; i++) {
      const v = data[i] / 128.0;
      const y = (v * height) / 2;

      if (i === 0) {
        context.moveTo(x, y);
      } else {
        context.lineTo(x, y);
      }
      x += sliceWidth;
    }

    context.lineTo(canvas.width, canvas.height / 2);

    context.stroke();
  }, [data, theme]);

  return <canvas ref={canvasRef} style={{ width: "100%", height: "100%" }} />;
};