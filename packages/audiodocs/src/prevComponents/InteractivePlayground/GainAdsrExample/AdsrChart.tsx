import React, { useRef, useEffect, FC, useState } from "react"; 
import { HANDLE_RADIUS, HIT_PADDING, MIN_TIME, SUSTAIN_HOLD_TIME, MAX_TIME, VISUAL_SCALE, ATTACK_MAX, RELEASE_MAX, DECAY_MAX } from "./constants";

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

    const rect = canvas.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;
    canvas.width = rect.width * dpr;
    canvas.height = rect.height * dpr;
    ctx.scale(dpr, dpr);
    const { width, height } = rect;

    const padding = 10;
    const chartWidth = width - 2 * padding;
    const chartHeight = height - 2 * padding;

    const timeToX = (time: number) =>
      padding + ((time * VISUAL_SCALE) / MAX_TIME) * chartWidth;
    const xToTime = (x: number) =>
      ((x - padding) / chartWidth) * (MAX_TIME / VISUAL_SCALE);

    const levelToY = (level: number) => padding + (1 - level) * chartHeight;
    const yToLevel = (y: number) => 1 - (y - padding) / chartHeight;

    const totalDuration = attack + decay + SUSTAIN_HOLD_TIME + release;

    const points = {
      start: { x: timeToX(0), y: levelToY(0) },
      attackEnd: { x: timeToX(attack), y: levelToY(1) },
      decayEnd: { x: timeToX(attack + decay), y: levelToY(sustain) },
      sustainEnd: {
        x: timeToX(attack + decay + SUSTAIN_HOLD_TIME),
        y: levelToY(sustain),
      },
      releaseEnd: {
        x: timeToX(totalDuration),
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

    const progressX = timeToX(totalDuration * playbackProgress);

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
      ctx.arc(p.x, p.y, HANDLE_RADIUS, 0, 2 * Math.PI);
      ctx.fillStyle = handleFill;
      ctx.fill();
    });

    const handleMouseDown = (e: MouseEvent) => {
      const rect = canvas.getBoundingClientRect();
      const mouseX = e.clientX - rect.left;
      const mouseY = e.clientY - rect.top;

      if (
        Math.hypot(points.attackEnd.x - mouseX, points.attackEnd.y - mouseY) <
        HANDLE_RADIUS + HIT_PADDING
      ) {
        setDraggingPoint("attack");
      } else if (
        Math.hypot(points.sustainEnd.x - mouseX, points.sustainEnd.y - mouseY) <
        HANDLE_RADIUS + HIT_PADDING
      ) {
        setDraggingPoint("sustain");
      } else if (
        Math.hypot(points.decayEnd.x - mouseX, points.decayEnd.y - mouseY) <
        HANDLE_RADIUS + HIT_PADDING
      ) {
        setDraggingPoint("decay");
      } else if (
        Math.hypot(points.releaseEnd.x - mouseX, points.releaseEnd.y - mouseY) <
        HANDLE_RADIUS + HIT_PADDING
      ) {
        setDraggingPoint("release");
      }
    };

    const handleMouseMove = (e: MouseEvent) => {
      if (!draggingPoint) return;
      const rect = canvas.getBoundingClientRect();
      const mouseX = e.clientX - rect.left;
      const mouseY = e.clientY - rect.top;

      const currentTime = xToTime(mouseX); // real seconds
      const currentLevel = yToLevel(mouseY);

      if (draggingPoint === "attack") {
        const clamped = Math.max(MIN_TIME, Math.min(currentTime, ATTACK_MAX));
        setAttack(clamped);
      } else if (draggingPoint === "decay") {
        const rawDecay = currentTime - attack;
        const clamped = Math.max(MIN_TIME, Math.min(rawDecay, DECAY_MAX));
        setDecay(clamped);
      } else if (draggingPoint === "sustain") {
        setSustain(Math.max(0, Math.min(1, currentLevel)));
      } else if (draggingPoint === "release") {
        const rawRelease = currentTime - (attack + decay + SUSTAIN_HOLD_TIME);
        const clamped = Math.max(MIN_TIME, Math.min(rawRelease, RELEASE_MAX));
        setRelease(clamped);
      }
    };

    const handleMouseUp = () => {
      setDraggingPoint(null);
    };
    const handleMouseLeave = () => {
      setDraggingPoint(null);
    };

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
