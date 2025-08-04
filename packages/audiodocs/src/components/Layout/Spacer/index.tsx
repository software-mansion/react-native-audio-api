import React, { memo } from "react";

interface SpacerProps {
  size?: number | string;
  className?: string;
}

const H: React.FC<SpacerProps> = memo(({ size, className }) => (
  <div style={size ? { width: size  } : { flex: 1 }} className={className} />
));

const V: React.FC<SpacerProps> = memo(({ size, className }) => (
  <div style={size ? { height: size  } : { flex: 1 }} className={className} />
))

const Spacer = {
  H,
  V,
}

export default Spacer;
