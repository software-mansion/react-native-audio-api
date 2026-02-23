import React, { memo, useCallback, useEffect, useRef } from 'react';
import styles from './styles.module.css';

export interface ResponsiveCanvasDrawParams {
  canvas: HTMLCanvasElement;
  context: CanvasRenderingContext2D;
  width: number;
  height: number;
  cssWidth: number;
  cssHeight: number;
  dpr: number;
}

export interface ResponsiveCanvasProps
  extends Omit<React.CanvasHTMLAttributes<HTMLCanvasElement>, 'height' | 'width'> {
  onDraw: (params: ResponsiveCanvasDrawParams) => void;
  aspectRatio?: number;
  height?: number;
  throttleMs?: number;
  containerClassName?: string;
  canvasRef?: React.Ref<HTMLCanvasElement>;
}

const DEFAULT_ASPECT_RATIO = 16 / 9;
const DEFAULT_THROTTLE_MS = 80;

const ResponsiveCanvas: React.FC<ResponsiveCanvasProps> = ({
  onDraw,
  aspectRatio = DEFAULT_ASPECT_RATIO,
  height,
  throttleMs = DEFAULT_THROTTLE_MS,
  className,
  containerClassName,
  canvasRef: externalCanvasRef,
  ...canvasProps
}) => {
  const containerRef = useRef<HTMLDivElement | null>(null);
  const internalCanvasRef = useRef<HTMLCanvasElement | null>(null);
  const rafIdRef = useRef<number | null>(null);
  const timeoutIdRef = useRef<number | null>(null);
  const lastDrawTimeRef = useRef(0);
  const drawRef = useRef<() => void>(() => undefined);

  const setCanvasRef = useCallback((element: HTMLCanvasElement | null) => {
    internalCanvasRef.current = element;

    if (typeof externalCanvasRef === 'function') {
      externalCanvasRef(element);
      return;
    }

    if (externalCanvasRef) {
      externalCanvasRef.current = element;
    }
  }, [externalCanvasRef]);

  const clearScheduledDraw = useCallback(() => {
    if (typeof window === 'undefined') {
      return;
    }

    if (timeoutIdRef.current !== null) {
      window.clearTimeout(timeoutIdRef.current);
      timeoutIdRef.current = null;
    }

    if (rafIdRef.current !== null) {
      window.cancelAnimationFrame(rafIdRef.current);
      rafIdRef.current = null;
    }
  }, []);

  const scheduleDraw = useCallback((immediate = false) => {
    if (typeof window === 'undefined') {
      return;
    }

    const runDraw = () => {
      rafIdRef.current = window.requestAnimationFrame(() => {
        rafIdRef.current = null;
        drawRef.current();
        lastDrawTimeRef.current = Date.now();
      });
    };

    if (immediate) {
      clearScheduledDraw();
      runDraw();
      return;
    }

    if (timeoutIdRef.current !== null || rafIdRef.current !== null) {
      return;
    }

    const elapsed = Date.now() - lastDrawTimeRef.current;
    const delay = Math.max(0, throttleMs - elapsed);

    timeoutIdRef.current = window.setTimeout(() => {
      timeoutIdRef.current = null;
      runDraw();
    }, delay);
  }, [clearScheduledDraw, throttleMs]);

  useEffect(() => {
    drawRef.current = () => {
      const container = containerRef.current;
      const canvas = internalCanvasRef.current;

      if (!container || !canvas) {
        return;
      }

      const cssWidth = Math.max(1, Math.floor(container.clientWidth));
      const ratio = aspectRatio > 0 ? aspectRatio : DEFAULT_ASPECT_RATIO;
      const cssHeight = Math.max(
        1,
        Math.floor(typeof height === 'number' ? height : cssWidth / ratio)
      );

      const dpr = (typeof window !== 'undefined' && window.devicePixelRatio)
        ? window.devicePixelRatio
        : 1;
      const width = Math.max(1, Math.floor(cssWidth * dpr));
      const computedHeight = Math.max(1, Math.floor(cssHeight * dpr));

      if (canvas.width !== width) {
        canvas.width = width;
      }
      if (canvas.height !== computedHeight) {
        canvas.height = computedHeight;
      }

      canvas.style.width = `${cssWidth}px`;
      canvas.style.height = `${cssHeight}px`;

      const context = canvas.getContext('2d');
      if (!context) {
        return;
      }

      onDraw({
        canvas,
        context,
        width,
        height: computedHeight,
        cssWidth,
        cssHeight,
        dpr,
      });
    };

    scheduleDraw(true);
  }, [aspectRatio, height, onDraw, scheduleDraw]);

  useEffect(() => {
    if (typeof window === 'undefined') {
      return undefined;
    }

    const handleResize = () => {
      scheduleDraw();
    };

    const observer = typeof ResizeObserver !== 'undefined'
      ? new ResizeObserver(handleResize)
      : null;

    if (observer && containerRef.current) {
      observer.observe(containerRef.current);
    }

    window.addEventListener('resize', handleResize);

    return () => {
      window.removeEventListener('resize', handleResize);
      observer?.disconnect();
      clearScheduledDraw();
    };
  }, [clearScheduledDraw, scheduleDraw]);

  const rootClassName = containerClassName
    ? `${styles.container} ${containerClassName}`
    : styles.container;

  const canvasClassName = className
    ? `${styles.canvas} ${className}`
    : styles.canvas;

  return (
    <div ref={containerRef} className={rootClassName}>
      <canvas ref={setCanvasRef} className={canvasClassName} {...canvasProps} />
    </div>
  );
};

export default memo(ResponsiveCanvas);
