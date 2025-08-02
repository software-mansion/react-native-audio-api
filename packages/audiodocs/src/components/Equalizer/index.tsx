import { useColorMode } from '@docusaurus/theme-common';
import React, { useEffect, useRef, useState } from 'react';


import styles from './styles.module.css';

interface EqualizerProps {
  eqBands: number[];
  setEqBands: (bands: number[]) => void;
}

const dotRadius = 8; // Doubled from 4 to account for 2x canvas size
const bandMin = -15;
const bandMax = 15;
const padding = 20; // Doubled from 10 to account for 2x canvas size

function lagrangeInterpolation(
  xTarget: number,
  xs: number[],
  ys: number[]
): number {
  const n = xs.length;
  let result = 0;

  for (let i = 0; i < n; i++) {
    let term = ys[i];
    for (let j = 0; j < n; j++) {
      if (j !== i) {
        term *= (xTarget - xs[j]) / (xs[i] - xs[j]);
      }
    }
    result += term;
  }

  return result;
}

const Equalizer: React.FC<EqualizerProps> = (props) => {
  const { eqBands = [], setEqBands } = props;
  const { colorMode } = useColorMode();

  const canvasRef = useRef<HTMLCanvasElement>(null);
  const canvasWrapperRef = useRef<HTMLDivElement>(null);
  const isDraggingRef = useRef<number | null>(null);

  const [curveType, setCurveType] = useState<"lagrange" | "catmull-rom">("catmull-rom");

  useEffect(() => {
    const canvas = canvasRef.current;

    if (!canvas) {
      return;
    }

    const ctx = canvas.getContext('2d');

    if (!ctx) {
      return;
    }

    if (canvas.width == 0) {
      return;
    }

    const width = canvas.width;
    const height = canvas.height;

    function drawColumns() {
      const columns = 5;
      const columnWidth = (width - 2 * padding) / columns;

      for (let i = 0; i < columns; i++) {
        const x = padding + i * columnWidth;

        // Create distinct gradient for each column
        const gradient = ctx.createLinearGradient(x, padding, x + columnWidth, height - padding);

        if (i % 2 === 0) {
          gradient.addColorStop(0, 'rgba(200, 200, 200, 0.3)');
          gradient.addColorStop(1, 'rgba(255, 255, 255, 0.1)');
        } else {
          gradient.addColorStop(0, 'rgba(0, 0, 0, 0.1)');
          gradient.addColorStop(1, 'rgba(0, 0, 0, 0.05)');
        }

        // Fill the column with gradient
        ctx.fillStyle = gradient;
        ctx.fillRect(x, padding, columnWidth, height - 2 * padding);
      }

      // Add single border around the entire column area
      ctx.strokeStyle = colorMode === 'dark' ? 'rgba(255, 255, 255, 0.2)' : 'rgba(0, 0, 0, 0.2)';
      ctx.lineWidth = 1; // Doubled from 0.5 to account for 2x canvas size
      ctx.strokeRect(padding, padding, width - 2 * padding, height - 2 * padding);
    }

    function drawCatmullRom(
      xs: number[],
      ys: number[],
      color: string
    ): void {
      ctx.beginPath();
      ctx.moveTo(xs[0], ys[0]);

      for (let i = 0; i < xs.length - 1; i++) {
        const x0 = i > 0 ? xs[i - 1] : xs[i];
        const y0 = i > 0 ? ys[i - 1] : ys[i];
        const x1 = xs[i];
        const y1 = ys[i];
        const x2 = xs[i + 1];
        const y2 = ys[i + 1];
        const x3 = i < xs.length - 2 ? xs[i + 2] : xs[i + 1];
        const y3 = i < ys.length - 2 ? ys[i + 2] : ys[i + 1];

        for (let t = 0; t <= 1; t += 0.02) { // Reduced from 0.05 to 0.02 for smoother curve
          const tt = t * t;
          const ttt = tt * t;

          const a0 = -0.5 * ttt + tt - 0.5 * t;
          const a1 = 1.5 * ttt - 2.5 * tt + 1.0;
          const a2 = -1.5 * ttt + 2.0 * tt + 0.5 * t;
          const a3 = 0.5 * ttt - 0.5 * tt;

          const x = a0 * x0 + a1 * x1 + a2 * x2 + a3 * x3;
          const y = a0 * y0 + a1 * y1 + a2 * y2 + a3 * y3;

          ctx.lineTo(x, y);
        }
      }

      ctx.strokeStyle = color;
      ctx.lineWidth = 2; // Doubled from 1 to account for 2x canvas size
      ctx.stroke();
    }

    function drawCurve(points: { x: number; y: number }[]) {
      const xs: number[] = points.map(p => p.x);
      const ys: number[] = points.map(p => p.y);

      // Calculate the middle line Y position (value 0)
      const yRange = height - padding * 2;
      const normalized = (bandMax - 0) / (bandMax - bandMin);
      const middleY = padding + normalized * yRange;

      if (curveType === "catmull-rom") {
        // Draw the filled area first
        ctx.beginPath();
        ctx.moveTo(xs[0], middleY);

        // Draw to the first point
        ctx.lineTo(xs[0], ys[0]);

        // Draw the curve
        for (let i = 0; i < xs.length - 1; i++) {
          const x0 = i > 0 ? xs[i - 1] : xs[i];
          const y0 = i > 0 ? ys[i - 1] : ys[i];
          const x1 = xs[i];
          const y1 = ys[i];
          const x2 = xs[i + 1];
          const y2 = ys[i + 1];
          const x3 = i < xs.length - 2 ? xs[i + 2] : xs[i + 1];
          const y3 = i < ys.length - 2 ? ys[i + 2] : ys[i + 1];

          for (let t = 0; t <= 1; t += 0.02) { // Reduced from 0.05 to 0.02 for smoother curve
            const tt = t * t;
            const ttt = tt * t;

            const a0 = -0.5 * ttt + tt - 0.5 * t;
            const a1 = 1.5 * ttt - 2.5 * tt + 1.0;
            const a2 = -1.5 * ttt + 2.0 * tt + 0.5 * t;
            const a3 = 0.5 * ttt - 0.5 * tt;

            const x = a0 * x0 + a1 * x1 + a2 * x2 + a3 * x3;
            const y = a0 * y0 + a1 * y1 + a2 * y2 + a3 * y3;

            ctx.lineTo(x, y);
          }
        }

        // Close the area by drawing back to middle line
        ctx.lineTo(xs[xs.length - 1], middleY);
        ctx.closePath();

        // Fill the area with semi-transparent color
        ctx.fillStyle = 'rgba(52, 152, 219, 0.2)';
        ctx.fill();

        // Now draw the curve line on top
        drawCatmullRom(xs, ys, '#3498db');
      } else {
        // Draw the filled area first for Lagrange interpolation
        ctx.beginPath();
        ctx.moveTo(xs[0], middleY);
        ctx.lineTo(xs[0], ys[0]);

        for (let x = xs[0]; x <= xs[xs.length - 1]; x += 1) { // Reduced from 2 to 1 for smoother curve
          const y = lagrangeInterpolation(x, xs, ys);
          ctx.lineTo(x, y);
        }

        ctx.lineTo(xs[xs.length - 1], middleY);
        ctx.closePath();

        // Fill the area
        ctx.fillStyle = colorMode === 'dark' ? 'rgba(170, 170, 170, 0.2)' : 'rgba(51, 51, 51, 0.2)';
        ctx.fill();

        // Draw the curve line on top
        ctx.beginPath();
        ctx.moveTo(xs[0], ys[0]);

        for (let x = xs[0]; x <= xs[xs.length - 1]; x += 1) { // Reduced from 2 to 1 for smoother curve
          const y = lagrangeInterpolation(x, xs, ys);
          ctx.lineTo(x, y);
        }

        ctx.strokeStyle = colorMode === "dark" ? "#aaa" : "#333";
        ctx.lineWidth = 2; // Doubled from 1 to account for 2x canvas size
        ctx.stroke();
      }
    }

    function drawPoints(points: { x: number; y: number }[] = []) {
      points.forEach(({ x, y }) => {
        ctx.beginPath();
        ctx.arc(x, y, dotRadius, 0, Math.PI * 2);
        ctx.fillStyle =  '#3498db';
        ctx.fill();
        ctx.closePath();
      });
    }

    function getPoints() {
      const yRange = height - padding * 2;

      return eqBands.map((p, idx) => {
        const x = padding + (idx / (eqBands.length - 1)) * (width - 2 * padding);

        const normalized = (bandMax - p) / (bandMax - bandMin);
        const y = padding + normalized * yRange;

        return { x, y };
      });
    }

    function draw() {
      ctx.clearRect(0, 0, width, height);

      drawColumns();

      const points = getPoints();

      drawCurve(points);
      drawPoints(points);
    }

    draw();
  }, [eqBands]);

  // Helper function to get mouse position relative to canvas
  const getMousePos = (canvas: HTMLCanvasElement, e: MouseEvent | React.MouseEvent): { x: number; y: number } => {
    const rect = canvas.getBoundingClientRect();
    const scaleX = canvas.width / rect.width;
    const scaleY = canvas.height / rect.height;
    return {
      x: (e.clientX - rect.left) * scaleX,
      y: (e.clientY - rect.top) * scaleY,
    };
  };

  // Helper function to get touch position relative to canvas
  const getTouchPos = (canvas: HTMLCanvasElement, e: TouchEvent): { x: number; y: number } => {
    const rect = canvas.getBoundingClientRect();
    const scaleX = canvas.width / rect.width;
    const scaleY = canvas.height / rect.height;
    return {
      x: (e.touches[0].clientX - rect.left) * scaleX,
      y: (e.touches[0].clientY - rect.top) * scaleY,
    };
  };

  // Helper function to find which point is being clicked
  const findPointIndex = (mouseX: number, mouseY: number): number | null => {
    const canvas = canvasRef.current;

    if (!canvas) {
      return null;
    }

    const width = canvas.width;
    const height = canvas.height;
    const yRange = height - padding * 2;

    for (let i = 0; i < eqBands.length; i++) {
      const x = padding + (i / (eqBands.length - 1)) * (width - 2 * padding);
      const normalized = (bandMax - eqBands[i]) / (bandMax - bandMin);
      const y = padding + normalized * yRange;

      const distance = Math.sqrt((mouseX - x) ** 2 + (mouseY - y) ** 2);
      if (distance <= dotRadius + 10) { // Doubled tolerance from 5 to 10 for 2x canvas size
        return i;
      }
    }
    return null;
  };

  // Helper function to update band value based on mouse position
  const updateBandValue = (pointIndex: number, mouseY: number) => {
    const canvas = canvasRef.current;

    if (!canvas) {
      return;
    }

    const height = canvas.height;
    const yRange = height - padding * 2;

    // Clamp mouseY to canvas bounds
    const clampedY = Math.max(padding, Math.min(mouseY, height - padding));

    // Convert Y position to band value
    const normalized = (clampedY - padding) / yRange;
    const newValue = bandMax - normalized * (bandMax - bandMin);
    const clampedValue = Math.max(bandMin, Math.min(bandMax, newValue));

    // Update the specific band
    const newBands = [...eqBands];
    newBands[pointIndex] = clampedValue;
    setEqBands(newBands);
  };

  // Mouse event handlers
  const handleMouseDown = (e: React.MouseEvent<HTMLCanvasElement>) => {
    const canvas = canvasRef.current;

    if (!canvas) {
      return;
    }

    const mousePos = getMousePos(canvas, e);
    const pointIndex = findPointIndex(mousePos.x, mousePos.y);

    if (pointIndex !== null) {
      isDraggingRef.current = pointIndex;
      canvas.style.cursor = 'grabbing';
    }
  };

  const handleMouseMove = (e: React.MouseEvent<HTMLCanvasElement>) => {
    const canvas = canvasRef.current;

    if (!canvas) {
      return;
    }

    const mousePos = getMousePos(canvas, e);

    if (isDraggingRef.current !== null) {
      updateBandValue(isDraggingRef.current, mousePos.y);
    } else {
      // Change cursor when hovering over points
      const pointIndex = findPointIndex(mousePos.x, mousePos.y);
      canvas.style.cursor = pointIndex !== null ? 'grab' : 'default';
    }
  };

  const handleMouseUp = () => {
    const canvas = canvasRef.current;

    if (!canvas) {
      return;
    }

    isDraggingRef.current = null;
    canvas.style.cursor = 'default';
  };

  const handleMouseLeave = () => {
    const canvas = canvasRef.current;

    if (!canvas) {
      return;
    }

    isDraggingRef.current = null;
    canvas.style.cursor = 'default';
  };

  const handleTouchStart = (e: React.TouchEvent<HTMLCanvasElement>) => {
    e.preventDefault();
    const canvas = canvasRef.current;

    if (!canvas) {
      return;
    }

    const touchPos = getTouchPos(canvas, e.nativeEvent);
    const pointIndex = findPointIndex(touchPos.x, touchPos.y);

    if (pointIndex !== null) {
      isDraggingRef.current = pointIndex;
    }
  };

  const handleTouchMove = (e: React.TouchEvent<HTMLCanvasElement>) => {
    e.preventDefault();
    const canvas = canvasRef.current;
    if (!canvas) return;

    if (isDraggingRef.current !== null) {
      const touchPos = getTouchPos(canvas, e.nativeEvent);
      updateBandValue(isDraggingRef.current, touchPos.y);
    }
  };

  const handleTouchEnd = (e: React.TouchEvent<HTMLCanvasElement>) => {
    e.preventDefault();
    isDraggingRef.current = null;
  };

  return (
    <div className={styles.equalizerContainer}>
      <div className={styles.equalizerCanvasWrapper} ref={canvasWrapperRef}>
        <canvas
          className={styles.equalizerCanvas}
          ref={canvasRef}
          width={550}
          height={250}
          style={{
            width: '275px',
            height: '125px',
          }}
          onMouseDown={handleMouseDown}
          onMouseMove={handleMouseMove}
          onMouseUp={handleMouseUp}
          onMouseLeave={handleMouseLeave}
          onTouchStart={handleTouchStart}
          onTouchMove={handleTouchMove}
          onTouchEnd={handleTouchEnd}
        />
      </div>
    </div>
  );
};

export default Equalizer;
