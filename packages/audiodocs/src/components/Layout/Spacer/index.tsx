import React, { memo } from "react";

interface SpacerProps {
  size?: number | string;
}

const H: React.FC<SpacerProps> = memo(({ size }) => (
  <div style={size ? { width: size  } : { flex: 1 }} />
));

const V: React.FC<SpacerProps> = memo(({ size }) => (
  <div style={size ? { height: size  } : { flex: 1 }} />
))

const Spacer = {
  H,
  V,
}

export default Spacer;
