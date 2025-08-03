import { useCallback, useEffect, useState, useRef } from "react";
import AudioManager from "./AudioManager";
import { frequencies } from "./Equalizer";

export interface EqualizerControl {
  frequency: number;
  gain: number;
  type: BiquadFilterType;
}

interface DragState {
  isDragging: boolean;
  dragIndex: number;
  startY: number;
  startGain: number;
}

const minFreq = frequencies[0].frequency;
const maxFreq = frequencies[frequencies.length - 1].frequency;
const logMin = Math.log10(minFreq);
const logMax = Math.log10(maxFreq);

const initialValues: EqualizerControl[] = frequencies.map(freq => ({
  frequency: freq.frequency,
  type: freq.type,
  gain: 0,
}));

export default function useEqualizerControls(canvasRef?: React.RefObject<HTMLCanvasElement>) {
  const [equalizerBands, setEqualizerBands] = useState<EqualizerControl[]>(initialValues);
  const dragState = useRef<DragState>({
    isDragging: false,
    dragIndex: -1,
    startY: 0,
    startGain: 0,
  });
  const canvasRect = useRef<DOMRect | null>(null);

  const updateGain = useCallback((index: number, gain: number) => {
    // Clamp gain between -12 and +12 dB
    const clampedGain = Math.max(-12, Math.min(12, gain));

    setEqualizerBands(prev => {
      const newBands = [...prev];
      newBands[index] = { ...newBands[index], gain: clampedGain };
      return newBands;
    });
  }, []);

  const getControlPointPosition = useCallback((frequency: number, gain: number, canvasRect: DOMRect, index: number) => {
    // Use the same drawing bounds logic as the drawing function
    const labelBoxSize = 32;
    const drawingWidth = canvasRect.width - labelBoxSize;
    const drawingHeight = canvasRect.height - labelBoxSize;
    const offsetX = 0; // drawing starts at x=0
    const offsetY = 0; // drawing starts at y=0

    const logFreq = Math.log10(frequency);
    const normalizedPos = (logFreq - logMin) / (logMax - logMin);
    let x = offsetX + (normalizedPos * drawingWidth);

    // Apply custom positioning - exactly like the drawing function:
    // Points 1,2,5 keep their current positions, point 3 goes to center + 96px, point 4 reflects point 2
    if (index === 0) {
      x += 96; // Point 1 (index 0) - keep current position
    } else if (index === 1) {
      x += 91; // Point 2 (index 1) - keep current position
    } else if (index === 2) {
      x = offsetX + drawingWidth / 2 + 59; // Point 3 (index 2) - center + 59px to the right
    } else if (index === 3) {
      x -= 18; // Point 4 (index 3) - reflect point 2's position
    } else if (index === 4) {
      x -= 96; // Point 5 (index 4) - keep current position
    }

    const normalizedGain = gain / 12; // -1 to 1 range
    const y = offsetY + drawingHeight / 2 - (normalizedGain * drawingHeight / 2);

    return { x, y };
  }, []);

  const findControlPointAtPosition = useCallback((x: number, y: number, rect: DOMRect) => {
    const controlPointRadius = 24; // 48px diameter = 24px radius

    for (let i = 0; i < equalizerBands.length; i++) {
      const band = equalizerBands[i];
      const pos = getControlPointPosition(band.frequency, band.gain, rect, i);

      const distance = Math.sqrt((x - pos.x) ** 2 + (y - pos.y) ** 2);
      if (distance <= controlPointRadius) {
        return i;
      }
    }
    return -1;
  }, [equalizerBands, getControlPointPosition]);  const handleStart = useCallback((clientX: number, clientY: number) => {
    if (!canvasRef?.current) {
      return;
    }

    const rect = canvasRef.current.getBoundingClientRect();
    canvasRect.current = rect;

    const x = clientX - rect.left;
    const y = clientY - rect.top;

    const index = findControlPointAtPosition(x, y, rect);
    if (index !== -1) {
      dragState.current = {
        isDragging: true,
        dragIndex: index,
        startY: clientY,
        startGain: equalizerBands[index].gain,
      };
    }
  }, [canvasRef, findControlPointAtPosition, equalizerBands]);

  const handleMove = useCallback((clientY: number) => {
    if (!dragState.current.isDragging || !canvasRect.current) {
      return;
    }

    const deltaY = dragState.current.startY - clientY;
    const labelBoxSize = 32;
    const drawingHeight = canvasRect.current.height - labelBoxSize;
    const gainChange = (deltaY / drawingHeight) * 24; // 24dB range using drawing height
    let newGain = dragState.current.startGain + gainChange;

    // Clamp gain to prevent control points from going outside the canvas boundaries
    // The control points have a 24px radius, so we need to account for that
    const controlPointRadius = 24;
    const maxGainForBounds = 12 * (1 - (2 * controlPointRadius) / drawingHeight);
    const minGainForBounds = -maxGainForBounds;

    // Apply stricter bounds to keep circles fully visible
    newGain = Math.max(minGainForBounds, Math.min(maxGainForBounds, newGain));

    updateGain(dragState.current.dragIndex, newGain);
  }, [updateGain]);

  const handleEnd = useCallback(() => {
    dragState.current.isDragging = false;
    dragState.current.dragIndex = -1;
    canvasRect.current = null;
  }, []);

  // Mouse event handlers
  const handleMouseDown = useCallback((e: MouseEvent) => {
    handleStart(e.clientX, e.clientY);
  }, [handleStart]);

  const handleMouseMove = useCallback((e: MouseEvent) => {
    handleMove(e.clientY);
  }, [handleMove]);

  const handleMouseUp = useCallback(() => {
    handleEnd();
  }, [handleEnd]);

  // Touch event handlers
  const handleTouchStart = useCallback((e: TouchEvent) => {
    if (e.touches.length === 1) {
      const touch = e.touches[0];
      handleStart(touch.clientX, touch.clientY);
    }
  }, [handleStart]);

  const handleTouchMove = useCallback((e: TouchEvent) => {
    if (e.touches.length === 1 && dragState.current.isDragging) {
      e.preventDefault(); // Prevent scrolling
      const touch = e.touches[0];
      handleMove(touch.clientY);
    }
  }, [handleMove]);

  const handleTouchEnd = useCallback(() => {
    handleEnd();
  }, [handleEnd]);

  // Setup global event listeners
  useEffect(() => {
    if (!canvasRef?.current) {
      return;
    }

    const canvas = canvasRef.current;

    // Add canvas event listeners
    canvas.addEventListener('mousedown', handleMouseDown);
    canvas.addEventListener('touchstart', handleTouchStart, { passive: false });

    // Add global event listeners
    document.addEventListener('mousemove', handleMouseMove);
    document.addEventListener('mouseup', handleMouseUp);
    document.addEventListener('touchmove', handleTouchMove, { passive: false });
    document.addEventListener('touchend', handleTouchEnd);
    document.addEventListener('touchcancel', handleTouchEnd);

    return () => {
      // Remove canvas event listeners
      canvas.removeEventListener('mousedown', handleMouseDown);
      canvas.removeEventListener('touchstart', handleTouchStart);

      // Remove global event listeners
      document.removeEventListener('mousemove', handleMouseMove);
      document.removeEventListener('mouseup', handleMouseUp);
      document.removeEventListener('touchmove', handleTouchMove);
      document.removeEventListener('touchend', handleTouchEnd);
      document.removeEventListener('touchcancel', handleTouchEnd);
    };
  }, [canvasRef, handleMouseDown, handleMouseMove, handleMouseUp, handleTouchStart, handleTouchMove, handleTouchEnd]);

  useEffect(() => {
    AudioManager.equalizer.setFrequencies(equalizerBands.map(band => band.frequency));
    AudioManager.equalizer.setGains(equalizerBands.map(band => band.gain));
  }, [equalizerBands]);

  return {
    equalizerBands,
    updateGain,
  };
}
