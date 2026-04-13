import React, { forwardRef, useCallback, useEffect, useRef } from 'react';
import type { ResponsiveCanvasDrawParams } from '@site/src/ui/ResponsiveCanvas';
import ResponsiveCanvas from '@site/src/ui/ResponsiveCanvas';

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
  const drawingRef = useRef<number | null>(null);
  const canvasContextRef = useRef<CanvasContext | null>(null);
  const renderingContextRef = useRef<RenderingContext | null>(null);

  const setCanvasRef = useCallback((element: HTMLCanvasElement | null) => {
    if (typeof ref === 'function') {
      ref(element);
    } else if (ref) {
      ref.current = element;
    }
  }, [ref]);

  const stopDrawing = useCallback(() => {
    if (drawingRef.current !== null) {
      cancelAnimationFrame(drawingRef.current);
      drawingRef.current = null;
    }
  }, []);

  const startDrawing = useCallback((canvasContext: CanvasContext) => {
    stopDrawing();

    if (!onDraw) {
      return;
    }

    canvasContextRef.current = canvasContext;
    renderingContextRef.current = prepareRenderingContext
      ? prepareRenderingContext(canvasContext)
      : ({} as RenderingContext);

    const draw = () => {
      if (!canvasContextRef.current) {
        return;
      }

      onDraw(canvasContextRef.current, renderingContextRef.current as RenderingContext);
      drawingRef.current = requestAnimationFrame(draw);
    };

    drawingRef.current = requestAnimationFrame(draw);
  }, [onDraw, prepareRenderingContext, stopDrawing]);

  const handleResponsiveCanvasDraw = useCallback(({ canvas, context }: ResponsiveCanvasDrawParams) => {
    startDrawing({ canvas, ctx: context });
  }, [startDrawing]);

  useEffect(() => {
    return () => {
      stopDrawing();
      setCanvasRef(null);
    };
  }, [setCanvasRef, stopDrawing]);

  return (
    <ResponsiveCanvas
      onDraw={handleResponsiveCanvasDraw}
      height={height}
      className={className}
      canvasRef={setCanvasRef}
    />
  );
});

export default Canvas;
