import { TouchEvent, useCallback, useRef, useState } from 'react';

const SWIPE_DISTANCE_RATIO = 0.3;

export default function useDrag(
  slideWidth: number,
  onSwipe: (step: number) => void
) {
  const [dragOffset, setDragOffset] = useState(0);
  const [isDragging, setIsDragging] = useState(false);
  const touchStartRef = useRef<number | null>(null);

  const endDrag = useCallback(() => {
    touchStartRef.current = null;
    setDragOffset(0);
    setIsDragging(false);
    document.body.style.overflowX = '';
  }, []);

  const onTouchStart = useCallback((event: TouchEvent) => {
    touchStartRef.current = event.targetTouches[0].clientX;
    setDragOffset(0);
    setIsDragging(true);
    document.body.style.overflowX = 'hidden';
  }, []);

  const onTouchMove = useCallback((event: TouchEvent) => {
    if (touchStartRef.current === null) {
      return;
    }

    setDragOffset(event.targetTouches[0].clientX - touchStartRef.current);
  }, []);

  const onTouchEnd = useCallback(() => {
    if (touchStartRef.current === null) {
      return;
    }

    const isSwipe =
      slideWidth > 0 && Math.abs(dragOffset) >= SWIPE_DISTANCE_RATIO * slideWidth;

    if (isSwipe) {
      onSwipe(dragOffset < 0 ? 1 : -1);
    }

    endDrag();
  }, [dragOffset, slideWidth, onSwipe, endDrag]);

  return {
    dragOffset,
    isDragging,
    dragHandlers: {
      onTouchStart,
      onTouchMove,
      onTouchEnd,
      onTouchCancel: endDrag,
    },
  };
}
