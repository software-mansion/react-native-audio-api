import React, { useEffect, useRef, useState } from 'react';

const CanvasSizer = ({ canvasHeight, children }) => {
  const containerRef = useRef(null);
  const [dimensions, setDimensions] = useState({ width: 0, height: 0 });

  useEffect(() => {
    const container = containerRef.current;

    if (!window.ResizeObserver) {
      console.error('ResizeObserver is not supported in your browser.');
      return;
    }

    const resizeObserver = new ResizeObserver((entries) => {
      for (let entry of entries) {
        setDimensions({
          width: entry.contentRect.width,
          height: entry.contentRect.height,
        });
      }
    });

    if (container) {
      resizeObserver.observe(container);
    }

    return () => {
      resizeObserver.disconnect();
    };
  }, []);

  return (
    <div ref={containerRef} style={{ height: canvasHeight }}>
      {children(dimensions)}
    </div>
  );
};

export default CanvasSizer;