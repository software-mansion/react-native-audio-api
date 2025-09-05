import React, { useRef, useEffect, FC, useState } from "react";

interface AdsrChartProps {
  attack: number;
  decay: number;
  sustain: number;
  release: number;
  setAttack: (v: number) => void;
  setDecay: (v: number) => void;
  setSustain: (v: number) => void;
  setRelease: (v: number) => void;
  theme: "light" | "dark";
  playbackProgress: number; // 0 to 1 indicating current playback position
}

const AdsrChart: FC<AdsrChartProps> = (props) => {
  const {
    attack,
    decay,
    sustain,
    release,
    setAttack,
    setDecay,
    setSustain,
    setRelease,
    theme,
    playbackProgress,
  } = props;
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const [draggingPoint, setDraggingPoint] = useState<string | null>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    const handleRadius = 4;
    const hitPadding = 8;
    const minTime = 0.01;
    const sustainHoldTime = 0.3;
    const maxTime = 4.0;

    const rect = canvas.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;
    canvas.width = rect.width * dpr;
    canvas.height = rect.height * dpr;
    ctx.scale(dpr, dpr);
    const { width, height } = rect;

    const padding = 20;
    const chartWidth = width - 2 * padding;
    const chartHeight = height - 2 * padding;

    const timeToX = (time: number) => padding + (time / maxTime) * chartWidth;
    const levelToY = (level: number) => padding + (1 - level) * chartHeight;
    const xToTime = (x: number) => ((x - padding) / chartWidth) * maxTime;
    const yToLevel = (y: number) => 1 - (y - padding) / chartHeight;

    const points = {
      start: { x: timeToX(0), y: levelToY(0) },
      attackEnd: { x: timeToX(attack), y: levelToY(1) },
      decayEnd: { x: timeToX(attack + decay), y: levelToY(sustain) },
      sustainEnd: {
        x: timeToX(attack + decay + sustainHoldTime),
        y: levelToY(sustain),
      },
      releaseEnd: {
        x: timeToX(attack + decay + sustainHoldTime + release),
        y: levelToY(0),
      },
    };

    ctx.clearRect(0, 0, width, height);

    ctx.fillStyle =
      theme === "dark" ? "rgba(167, 139, 250, 0.4)" : "rgba(139, 92, 246, 0.6)";
    ctx.beginPath();
    ctx.moveTo(points.start.x, points.start.y);
    ctx.lineTo(points.attackEnd.x, points.attackEnd.y);
    ctx.lineTo(points.decayEnd.x, points.decayEnd.y);
    ctx.lineTo(points.sustainEnd.x, points.sustainEnd.y);
    ctx.lineTo(points.releaseEnd.x, points.releaseEnd.y);
    ctx.closePath();
    ctx.fill();

    ctx.save();

    ctx.beginPath();
    ctx.moveTo(points.start.x, points.start.y);
    ctx.lineTo(points.attackEnd.x, points.attackEnd.y);
    ctx.lineTo(points.decayEnd.x, points.decayEnd.y);
    ctx.lineTo(points.sustainEnd.x, points.sustainEnd.y);
    ctx.lineTo(points.releaseEnd.x, points.releaseEnd.y);
    ctx.closePath();
    ctx.clip();

    const progressX = padding + playbackProgress * chartWidth;

    ctx.fillStyle =
      theme === "dark" ? "rgba(167, 139, 250, 0.4)" : "rgba(139, 92, 246, 0.6)";

    ctx.fillRect(0, 0, progressX, height);

    ctx.restore();

    const handleFill =
      theme === "dark" ? "rgba(193, 189, 204, 1)" : "rgba(193, 189, 204, 1)";
    ctx.lineWidth = 1.5;

    [
      points.attackEnd,
      points.decayEnd,
      points.releaseEnd,
      points.sustainEnd,
    ].forEach((p) => {
      ctx.beginPath();
      ctx.arc(p.x, p.y, handleRadius, 0, 2 * Math.PI);
      ctx.fillStyle = handleFill;
      ctx.fill();
    });

    const handleMouseDown = (e: MouseEvent) => {
      const rect = canvas.getBoundingClientRect();
      const mouseX = e.clientX - rect.left;
      const mouseY = e.clientY - rect.top;

      if (
        Math.hypot(points.attackEnd.x - mouseX, points.attackEnd.y - mouseY) <
        handleRadius + hitPadding
      ) {
        setDraggingPoint("attack");
      } else if (
        Math.hypot(points.sustainEnd.x - mouseX, points.sustainEnd.y - mouseY) <
        handleRadius + hitPadding
      ) {
        setDraggingPoint("sustain");
      } else if (
        Math.hypot(points.decayEnd.x - mouseX, points.decayEnd.y - mouseY) <
        handleRadius + hitPadding
      ) {
        setDraggingPoint("decay");
      } else if (
        Math.hypot(points.releaseEnd.x - mouseX, points.releaseEnd.y - mouseY) <
        handleRadius + hitPadding
      ) {
        setDraggingPoint("release");
      }
    };

    const handleMouseMove = (e: MouseEvent) => {
      if (!draggingPoint) return;
      const rect = canvas.getBoundingClientRect();
      const mouseX = e.clientX - rect.left;
      const mouseY = e.clientY - rect.top;

      const currentTime = xToTime(mouseX);
      const currentLevel = yToLevel(mouseY);

      if (draggingPoint === "attack") {
        setAttack(
          Math.max(
            minTime,
            Math.min(currentTime, maxTime - (decay + sustainHoldTime + release))
          )
        );
      } else if (draggingPoint === "decay") {
        setDecay(Math.max(minTime, currentTime - attack));
      } else if (draggingPoint === "sustain") {
        setSustain(Math.max(0, Math.min(1, currentLevel)));
      } else if (draggingPoint === "release") {
        setRelease(
          Math.max(minTime, currentTime - (attack + decay + sustainHoldTime))
        );
      }
    };

    const handleMouseUp = () => setDraggingPoint(null);
    const handleMouseLeave = () => setDraggingPoint(null);

    canvas.addEventListener("mousedown", handleMouseDown);
    canvas.addEventListener("mousemove", handleMouseMove);
    window.addEventListener("mouseup", handleMouseUp);
    canvas.addEventListener("mouseleave", handleMouseLeave);

    return () => {
      canvas.removeEventListener("mousedown", handleMouseDown);
      canvas.removeEventListener("mousemove", handleMouseMove);
      window.removeEventListener("mouseup", handleMouseUp);
      canvas.removeEventListener("mouseleave", handleMouseLeave);
    };
  }, [
    attack,
    decay,
    sustain,
    release,
    theme,
    draggingPoint,
    setAttack,
    setDecay,
    setSustain,
    setRelease,
    playbackProgress,
  ]);

  return (
    <canvas
      ref={canvasRef}
      style={{
        width: "100%",
        height: "100%",
        cursor: draggingPoint ? "grabbing" : "grab",
      }}
    />
  );
};

export default AdsrChart;
