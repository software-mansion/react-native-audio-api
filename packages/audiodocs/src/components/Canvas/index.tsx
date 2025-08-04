import React, { useRef, useState, useLayoutEffect, memo, forwardRef } from 'react';

import styles from './styles.module.css';
import clsx from 'clsx';

export interface CanvasContext {
  canvas: HTMLCanvasElement;
  ctx: CanvasRenderingContext2D;
}

interface CanvasProps<RenderingContext> {
  height?: number;
  prepareRenderingContext?: (canvasContext: CanvasContext) => RenderingContext;
  onDraw?: (canvasContext: CanvasContext, renderingContext: RenderingContext) => void
  className?: string;
}

const Canvas = forwardRef<HTMLCanvasElement, CanvasProps<any>>(function Canvas<RenderingContext>(
  props: CanvasProps<RenderingContext>,
  ref: React.ForwardedRef<HTMLCanvasElement>
) {
  const { height = 250, onDraw, prepareRenderingContext, className } = props;

  const canvasWrapperRef = useRef<HTMLDivElement>(null);
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const drawingRef = useRef<number | null>(null);

  const [width, setWidth] = useState(0);

  // Combine internal ref with forwarded ref
  const setCanvasRef = (element: HTMLCanvasElement | null) => {
    canvasRef.current = element;
    if (typeof ref === 'function') {
      ref(element);
    } else if (ref) {
      ref.current = element;
    }
  };

  useLayoutEffect(() => {
    const canvas = canvasRef.current;

    if (!canvas || !canvas.width || !onDraw) {
      return;
    }

    const ctx = canvas.getContext('2d');

    if (!ctx) {
      return;
    }

    const canvasContext: CanvasContext = { canvas, ctx };
    const renderingContext = prepareRenderingContext ? prepareRenderingContext(canvasContext) : {} as RenderingContext;

    function draw() {
      onDraw(canvasContext, renderingContext);
      drawingRef.current = requestAnimationFrame(draw);
    }

    drawingRef.current = requestAnimationFrame(draw);

    return () => {
      if (drawingRef.current) {
        cancelAnimationFrame(drawingRef.current);
      }
    };
  }, [onDraw, prepareRenderingContext, width]);

  useLayoutEffect(() => {
    const updateWidth = () => {
      if (canvasWrapperRef.current) {
        setWidth(canvasWrapperRef.current.clientWidth);
      }
    };

    updateWidth();
    window.addEventListener('resize', updateWidth);

    return () => {
      window.removeEventListener('resize', updateWidth);
    };
  }, []);

  return (
    <div ref={canvasWrapperRef}>
      <canvas className={clsx(styles.canvas, className)} ref={setCanvasRef} width={width} height={height}></canvas>
    </div>
  );
});

export default Canvas;
