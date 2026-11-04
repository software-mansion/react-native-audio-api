import { Skia } from '@shopify/react-native-skia';

/**
 * SkSL port of the voice orb fragment shader from rn-audio-shader
 * (`shaderEffects/voiceOrb.ts`, written in TypeGPU): a disc of liquid colour
 * whose ripples, drift and glow grow with the voice level. The palette is a
 * uniform here so the orb can tint toward whoever is speaking.
 */
const source = `
uniform float2 resolution;
uniform float time;
uniform float amplitude;
uniform float radius;
uniform float glow;
uniform float brightness;
uniform float pulse;
uniform float3 baseColor;
uniform float3 highlightColor;
uniform float3 accentColor;
uniform float3 softAccentColor;
uniform float3 shadowColor;

half4 main(float2 fragCoord) {
  float2 uv = fragCoord / resolution;
  uv.y = 1.0 - uv.y;
  float minResolution = max(1.0, min(resolution.x, resolution.y));
  float2 centered = (uv * 2.0 - 1.0) * (resolution / minResolution);
  float distanceFromCenter = length(centered);

  float mask = 1.0 - smoothstep(radius - 0.012, radius + 0.018, distanceFromCenter);
  float inner = 1.0 - smoothstep(radius * 0.32, radius, distanceFromCenter);
  float edgeGlow = (1.0 - smoothstep(radius, radius + 0.18, distanceFromCenter)) *
    smoothstep(radius * 0.48, radius, distanceFromCenter);

  float activity = clamp((amplitude - 0.18) / 1.7, 0.0, 1.0);
  float liquidActivity = smoothstep(0.04, 0.48, activity);
  float voicePulse = pulse * liquidActivity;
  float breathe = sin(time);
  float voiceRipple = sin(time * 1.8 + distanceFromCenter * 7.5 + liquidActivity * 0.9 + voicePulse * 1.2) *
    (liquidActivity * 0.72 + voicePulse * 0.38);

  float2 baseOrbUv = centered * ((1.0 + amplitude * 0.028 + breathe * amplitude * 0.009 +
    voiceRipple * (0.036 + voicePulse * 0.034)) * mix(1.14, 0.84, inner));
  float2 voiceDrift = float2(
    sin(centered.y * 4.4 + time * 1.35),
    cos(centered.x * 4.1 - time * 1.18)
  ) * (inner * (liquidActivity * 0.048 + voicePulse * 0.078));
  float2 orbUv = baseOrbUv + voiceDrift;

  float phase = -time * 0.5 - liquidActivity * 0.32 - voicePulse * 0.24;
  float wave = liquidActivity * 0.48 + voicePulse * 0.44;
  for (int i = 0; i < 8; i++) {
    float fi = float(i);
    wave += cos(fi - phase - wave * orbUv.x - voiceRipple * (0.26 + voicePulse * 0.22));
    phase += sin(orbUv.y * fi + wave + liquidActivity * 0.42 + voicePulse * 0.52);
  }

  float3 flow = float3(
    cos(orbUv.x * 2.6 + phase) * 0.52 + 0.48,
    cos(orbUv.y * 2.4 + wave * 0.34 - phase * 0.22) * 0.52 + 0.48,
    cos((orbUv.x + orbUv.y) * 1.8 + wave * 0.28 + phase * 0.18) * 0.5 + 0.5
  );
  float flowA = smoothstep(0.18, 0.9, flow.r);
  float flowB = smoothstep(0.22, 0.88, flow.g);
  float3 warmAccent = mix(accentColor, baseColor, smoothstep(0.25, 0.88, flow.r));

  float3 color = mix(baseColor, softAccentColor, flowB * 0.46);
  color = mix(color, highlightColor, inner * flowA * 0.18);
  color = mix(color, shadowColor, (1.0 - inner) * 0.16 + flow.b * 0.025);
  color += warmAccent * (inner * (liquidActivity * 0.04 + voicePulse * 0.065));
  color += accentColor * (flowB * inner * (0.02 + liquidActivity * 0.014 + voicePulse * 0.032));
  color += shadowColor * (edgeGlow * glow * 0.16);
  color += baseColor * (pow(inner, 3.0) * 0.035);
  color *= 0.68 + brightness * 0.22 + mask * 0.2 + liquidActivity * 0.03 + voicePulse * 0.045;

  float alpha = clamp(mask + edgeGlow * 0.18 * glow, 0.0, 1.0);
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

export type OrbPalette = {
  base: number[];
  highlight: number[];
  accent: number[];
  softAccent: number[];
  shadow: number[];
};

function rgb(hex: string): number[] {
  return [1, 3, 5].map(
    (start) => Number.parseInt(hex.slice(start, start + 2), 16) / 255
  );
}

/** The orb's original colours, for the assistant's voice. */
export const assistantPalette: OrbPalette = {
  base: rgb('#38ACDD'),
  highlight: rgb('#FF6259'),
  accent: rgb('#FFD61E'),
  softAccent: rgb('#DFF2EC'),
  shadow: rgb('#001A72'),
};

/** Shifted toward green while the user speaks. */
export const userPalette: OrbPalette = {
  base: rgb('#57B495'),
  highlight: rgb('#38ACDD'),
  accent: rgb('#FFE04B'),
  softAccent: rgb('#B1DFD0'),
  shadow: rgb('#001A72'),
};
