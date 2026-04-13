import React, { FC, useCallback, useEffect, useRef, useState } from "react";
import {
  ATTACK_MAX,
  DECAY_MAX,
  HANDLE_RADIUS,
  HIT_PADDING,
  MAX_TIME,
  MIN_TIME,
  RELEASE_MAX,
  SUSTAIN_HOLD_TIME,
  VISUAL_SCALE,
} from "./constants";

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

type DraggingPoint = "attack" | "decay" | "sustain" | "release";

const PADDING = 10;

const getGeometry = (
  width: number,
  height: number,
  attack: number,
  decay: number,
  sustain: number,
  release: number
) => {
  const chartWidth = Math.max(1, width - 2 * PADDING);
  const chartHeight = Math.max(1, height - 2 * PADDING);

  const timeToX = (time: number) =>
    PADDING + ((time * VISUAL_SCALE) / MAX_TIME) * chartWidth;
  const xToTime = (x: number) =>
    ((x - PADDING) / chartWidth) * (MAX_TIME / VISUAL_SCALE);

  const levelToY = (level: number) => PADDING + (1 - level) * chartHeight;
  const yToLevel = (y: number) => 1 - (y - PADDING) / chartHeight;

  const totalDuration = attack + decay + SUSTAIN_HOLD_TIME + release;
  const points = {
    start: { x: timeToX(0), y: levelToY(0) },
    attackEnd: { x: timeToX(attack), y: levelToY(1) },
    decayEnd: { x: timeToX(attack + decay), y: levelToY(sustain) },
    sustainEnd: {
      x: timeToX(attack + decay + SUSTAIN_HOLD_TIME),
      y: levelToY(sustain),
    },
    releaseEnd: { x: timeToX(totalDuration), y: levelToY(0) },
  };

  return { points, totalDuration, timeToX, xToTime, yToLevel };
};

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
  const draggingPointRef = useRef<DraggingPoint | null>(null);
  const [isDragging, setIsDragging] = useState(false);

  const getMousePosition = useCallback((clientX: number, clientY: number) => {
    const canvas = canvasRef.current;
    if (!canvas) {
      return null;
    }
    const rect = canvas.getBoundingClientRect();
    return {
      x: clientX - rect.left,
      y: clientY - rect.top,
      width: rect.width,
      height: rect.height,
    };
  }, []);

  const findDragPoint = useCallback((mouseX: number, mouseY: number, width: number, height: number) => {
    const { points } = getGeometry(width, height, attack, decay, sustain, release);

    if (Math.hypot(points.attackEnd.x - mouseX, points.attackEnd.y - mouseY) < HANDLE_RADIUS + HIT_PADDING) {
      return "attack" as const;
    }
    if (Math.hypot(points.sustainEnd.x - mouseX, points.sustainEnd.y - mouseY) < HANDLE_RADIUS + HIT_PADDING) {
      return "sustain" as const;
    }
    if (Math.hypot(points.decayEnd.x - mouseX, points.decayEnd.y - mouseY) < HANDLE_RADIUS + HIT_PADDING) {
      return "decay" as const;
    }
    if (Math.hypot(points.releaseEnd.x - mouseX, points.releaseEnd.y - mouseY) < HANDLE_RADIUS + HIT_PADDING) {
      return "release" as const;
    }

    return null;
  }, [attack, decay, sustain, release]);

  const updateByPointer = useCallback((point: DraggingPoint, mouseX: number, mouseY: number, width: number, height: number) => {
    const { xToTime, yToLevel } = getGeometry(width, height, attack, decay, sustain, release);

    const currentTime = xToTime(mouseX); // real seconds
    const currentLevel = yToLevel(mouseY);

    if (point === "attack") {
      const clamped = Math.max(MIN_TIME, Math.min(currentTime, ATTACK_MAX));
      setAttack(clamped);
      return;
    }

    if (point === "decay") {
      const rawDecay = currentTime - attack;
      const clamped = Math.max(MIN_TIME, Math.min(rawDecay, DECAY_MAX));
      setDecay(clamped);
      return;
    }

    if (point === "sustain") {
      setSustain(Math.max(0, Math.min(1, currentLevel)));
      return;
    }

    const rawRelease = currentTime - (attack + decay + SUSTAIN_HOLD_TIME);
    const clamped = Math.max(MIN_TIME, Math.min(rawRelease, RELEASE_MAX));
    setRelease(clamped);
  }, [attack, decay, sustain, release, setAttack, setDecay, setSustain, setRelease]);

  const handlePointerDown = useCallback((e: React.PointerEvent<HTMLCanvasElement>) => {
    const position = getMousePosition(e.clientX, e.clientY);
    if (!position) {
      return;
    }

    const nextDraggingPoint = findDragPoint(position.x, position.y, position.width, position.height);
    if (!nextDraggingPoint) {
      return;
    }

    draggingPointRef.current = nextDraggingPoint;
    setIsDragging(true);
    e.currentTarget.setPointerCapture(e.pointerId);
    updateByPointer(nextDraggingPoint, position.x, position.y, position.width, position.height);
  }, [findDragPoint, getMousePosition, updateByPointer]);

  const handlePointerMove = useCallback((e: React.PointerEvent<HTMLCanvasElement>) => {
    const draggingPoint = draggingPointRef.current;
    if (!draggingPoint) {
      return;
    }

    const position = getMousePosition(e.clientX, e.clientY);
    if (!position) {
      return;
    }

    updateByPointer(draggingPoint, position.x, position.y, position.width, position.height);
  }, [getMousePosition, updateByPointer]);

  const stopDragging = useCallback((e: React.PointerEvent<HTMLCanvasElement>) => {
    if (!draggingPointRef.current) {
      return;
    }
    draggingPointRef.current = null;
    setIsDragging(false);
    if (e.currentTarget.hasPointerCapture(e.pointerId)) {
      e.currentTarget.releasePointerCapture(e.pointerId);
    }
  }, []);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    const rect = canvas.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;
    canvas.width = rect.width * dpr;
    canvas.height = rect.height * dpr;
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    const { width, height } = rect;

    const { points, totalDuration, timeToX } = getGeometry(
      width,
      height,
      attack,
      decay,
      sustain,
      release
    );

    ctx.clearRect(0, 0, width, height);

    ctx.fillStyle =
      theme === "dark" ? "rgba(255, 119, 116, 0.4)" : "rgba(255, 119, 116, 0.6)";
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
      theme === "dark" ? "rgba(255, 119, 116, 0.4)" : "rgba(255, 119, 116, 0.6)";

    ctx.fillRect(0, 0, progressX, height);

    ctx.restore();

    const handleFill =
      theme === "dark" ? "var(--swm-off-white)" : "var(--swm-off-white)";
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
  }, [attack, decay, sustain, release, theme, playbackProgress]);

  return (
    <canvas
      ref={canvasRef}
      onPointerDown={handlePointerDown}
      onPointerMove={handlePointerMove}
      onPointerUp={stopDragging}
      onPointerCancel={stopDragging}
      onLostPointerCapture={stopDragging}
      style={{
        width: "100%",
        height: "100%",
        cursor: isDragging ? "grabbing" : "grab",
        touchAction: "none",
      }}
    />
  );
};

export default AdsrChart;
