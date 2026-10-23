import { RefObject, useEffect, useRef, useState } from 'react';

/**
 * Tracks the rendered width of the element the returned ref is attached to. The width is `0`
 * until the first layout, so callers should treat `0` as "unknown" rather than "narrow".
 */
export default function useContainerWidth<T extends HTMLElement>(): [RefObject<T | null>, number] {
  const ref = useRef<T | null>(null);
  const [width, setWidth] = useState(0);

  useEffect(() => {
    const element = ref.current;
    if (!element) {
      return undefined;
    }

    setWidth(element.clientWidth);

    if (typeof ResizeObserver === 'undefined') {
      return undefined;
    }

    const observer = new ResizeObserver((entries) => {
      const entry = entries[0];
      if (entry) {
        setWidth(entry.contentRect.width);
      }
    });
    observer.observe(element);

    return () => observer.disconnect();
  }, []);

  return [ref, width];
}
