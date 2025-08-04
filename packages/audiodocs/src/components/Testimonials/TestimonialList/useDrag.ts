
import { TouchEvent, useCallback, useEffect, useRef, useState } from 'react';
import testimonials from './testimonials';

function parseTranslateX(transform: string): number | null {
  const match = transform.match(/translateX\((-?\d+.?\d*)px\)/);
  return match ? parseFloat(match[1]) : null;
}

function minMax(value: number, min: number, max: number): number {
  return Math.max(min, Math.min(value, max));
}

export default function useDrag(currentOffset: number, setCurrentOffset: (offset: number) => void, activeIndex: number, onSetActiveSlide: (index: number) => void) {
  const draggableRef = useRef<HTMLDivElement>(null);

  const [isDragging, setIsDragging] = useState(false);
  const touchStartRef = useRef<number | null>(null);
  const touchEndRef = useRef<number | null>(null);
  const [initialOffset, setInitialOffset] = useState(0);

  const onTouchStart = useCallback((e: TouchEvent) => {
    if (!draggableRef.current) {
      return;
    }

    touchStartRef.current = e.targetTouches[0].clientX;
    touchEndRef.current = null;
    setIsDragging(true);

    draggableRef.current.style.transition = 'none';
    setInitialOffset(parseTranslateX(draggableRef.current.style.transform) || 0);

    document.body.style.overflowX = 'hidden';
  }, []);

  const onTouchMove = useCallback((e: TouchEvent) => {
    if (!isDragging || !touchStartRef.current || !draggableRef.current) {
      return;
    }

    const currentX = e.targetTouches[0].clientX;
    touchEndRef.current = currentX;

    const maxOffset = draggableRef.current.clientWidth - (document.querySelector(`.testimonialContainer-${activeIndex}`)?.clientWidth || 0);

    const offset = minMax(currentX - touchStartRef.current + initialOffset, -maxOffset, 0);
    const container = draggableRef.current;

    if (container) {
      setCurrentOffset(-offset);
    }
  }, [isDragging, initialOffset, activeIndex, setCurrentOffset]);

  const onTouchEnd = useCallback(() => {
    if (!isDragging || !touchStartRef.current || !touchEndRef.current) {
      return;
    }

    const distance = touchStartRef.current - touchEndRef.current;
    const minDistance = 0.30 * (document.querySelector(`.testimonialContainer-0`)?.clientWidth || 0);
    const isLeftSwipe = distance >= minDistance;
    const isRightSwipe = distance <= -minDistance;

    let newIndex = activeIndex;

    if (isLeftSwipe) {
      newIndex = Math.min(activeIndex + 1, testimonials.length - 1);
    } else if (isRightSwipe) {
      newIndex = Math.max(activeIndex - 1, 0);
    }


    document.body.style.overflowX = '';
    draggableRef.current!.style.transition = 'transform 300ms ease-out';

    if (newIndex !== activeIndex) {
      onSetActiveSlide(newIndex);
    } else {
      setCurrentOffset(initialOffset);
    }

    touchStartRef.current = null;
    touchEndRef.current = null;
    setInitialOffset(0);

    setIsDragging(false);
  }, [isDragging, activeIndex, onSetActiveSlide, setCurrentOffset, initialOffset, currentOffset]);

  const onTouchCancel = useCallback(() => {
    setIsDragging(false);
    document.body.style.overflowX = '';

    touchStartRef.current = null;
    touchEndRef.current = null;

    if (draggableRef.current) {
      draggableRef.current.style.transition = 'transform 300ms ease-out';
      setCurrentOffset(initialOffset);
    }

    setInitialOffset(0);
  }, []);

  return {
    ref: draggableRef,
    onTouchStart,
    onTouchMove,
    onTouchEnd,
    onTouchCancel,
  };
}
