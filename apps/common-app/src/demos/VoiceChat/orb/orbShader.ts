import { Skia } from '@shopify/react-native-skia';

/**
 * SkSL port of the aec-workshop promo orb (`promo-orb/shader.ts`, TypeGPU): a
 * disc of liquid colour whose motion and halo follow the session state. The
 * accents are decorative, not a measurement of loudness.
 */
const source = `
uniform float2 resolution;
uniform float time;
uniform float listening;
uniform float speaking;
uniform float edgeWidth;
uniform float3 primaryColor;
uniform float3 accentColor;
uniform float3 highlightColor;

half4 main(float2 fragCoord) {
  float2 uv = fragCoord / resolution;
  uv.y = 1.0 - uv.y;
  float minResolution = max(1.0, min(resolution.x, resolution.y));
  float2 centered = (uv * 2.0 - 1.0) * (resolution / minResolution);
  float distance = length(centered);

  float pulse = 0.5 + 0.5 * sin(time * 4.2);
  float radius = 0.76 + listening * (0.02 + pulse * 0.045) +
    speaking * (0.025 + pulse * 0.06);
  float glow = 0.08 + listening * 0.42 + speaking * 0.9;
  float mask = 1.0 - smoothstep(radius - edgeWidth, radius + edgeWidth, distance);
  float inner = 1.0 - smoothstep(radius * 0.32, radius, distance);
  float edgeGlow = (1.0 - smoothstep(radius, radius + 0.06, distance)) *
    smoothstep(radius * 0.48, radius, distance);
  float2 orbUv = centered * ((1.0 + 0.18 * 0.028 + sin(time) * 0.18 * 0.009 +
    (listening * 0.015 + speaking * 0.12) * sin(distance * 12.0 - time * 3.5)) *
    mix(1.14, 0.84, inner));

  float phase = -time * 0.5;
  float wave = 0.0;
  for (int i = 0; i < 8; i++) {
    float fi = float(i);
    wave += cos(fi - phase - wave * orbUv.x);
    phase += sin(orbUv.y * fi + wave);
  }

  float3 flow = float3(
    cos(orbUv.x * 2.6 + phase) * 0.52 + 0.48,
    cos(orbUv.y * 2.4 + wave * 0.34 - phase * 0.22) * 0.52 + 0.48,
    cos((orbUv.x + orbUv.y) * 1.8 + wave * 0.28 + phase * 0.18) * 0.5 + 0.5
  );
  float flowA = smoothstep(0.18, 0.9, flow.r);
  float flowB = smoothstep(0.22, 0.88, flow.g);
  float3 color = mix(primaryColor, accentColor, flowB * 0.8);
  color = mix(color, highlightColor, inner * flowA * 0.85);
  color *= 0.8 + mask * 0.2;
  color += primaryColor * (edgeGlow * glow * 0.06);

  float alpha = clamp(mask + edgeGlow * 0.06 * glow, 0.0, 1.0);
  return half4(half3(clamp(color, 0.0, 1.0) * alpha), half(alpha));
}
`;

export const orbEffect = (() => {
  const effect = Skia.RuntimeEffect.Make(source);
  if (!effect) {
    throw new Error('Voice orb shader failed to compile');
  }
  return effect;
})();

/**
 * Half-width of the disc's anti-aliased edge, in the shader's centred
 * coordinates where the canvas spans -1..1. Stands in for `fwidth(distance)`,
 * which Skia runtime effects don't provide: a radial field changes by one
 * coordinate unit per `size / 2` device pixels.
 */
export function orbEdgeWidth(sizeInDevicePixels: number): number {
  return Math.max(0.003, (2 / Math.max(1, sizeInDevicePixels)) * 0.75);
}
