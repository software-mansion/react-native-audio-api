interface EffectCodeParts {
  sampleVariable: string;
  effectLabel: string;
  enabled: boolean;
  /** Lines that create and configure the node(s). */
  setup: string;
  /** Lines that connect the graph while the effect is active. */
  wiring: string;
  /** Lines that connect the graph while the effect is bypassed; defaults to a direct connection. */
  bypassedWiring?: string;
  /** Helper functions printed after the graph code. */
  helpers?: string;
}

export function buildEffectCode({
  sampleVariable,
  effectLabel,
  enabled,
  setup,
  wiring,
  bypassedWiring,
  helpers,
}: EffectCodeParts): string {
  const defaultBypassedWiring = `// ${effectLabel} is bypassed: the source plays straight into the destination
source.connect(ctx.destination);`;

  const sections = [
    `import { AudioContext } from 'react-native-audio-api';

const ctx = new AudioContext();
const source = ctx.createBufferSource();
source.buffer = ${sampleVariable}; // any AudioBuffer, e.g. from ctx.decodeAudioData()
source.loop = true;`,
    setup,
    `${enabled ? wiring : (bypassedWiring ?? defaultBypassedWiring)}
source.start();`,
  ];

  if (helpers) {
    sections.push(helpers);
  }

  return sections.join('\n\n');
}
