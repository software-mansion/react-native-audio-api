import React, { useRef, useState, useEffect, MouseEvent } from 'react';
import styles from './OscillatorSquare.module.css'

interface LFO {
  lfo: OscillatorNode;
  lfoAmplifier: GainNode;
  setFrequency: (value: number) => void;
  setAmplify: (value: number) => void;
  connect: (node: AudioNode | AudioParam) => void;
}

const squareSize = 200;

function createLFO(aCtx: AudioContext): LFO {
  const lfo = aCtx.createOscillator();
  const lfoAmplifier = aCtx.createGain();

  lfo.connect(lfoAmplifier);
  lfo.start();

  return {
    lfo,
    lfoAmplifier,
    setFrequency: (value: number) => {
      lfo.frequency.value = value;
    },
    setAmplify: (value: number) => {
      lfoAmplifier.gain.value = value;
    },
    connect: (node: AudioNode | AudioParam) => {
      if (node instanceof AudioNode) {
        lfoAmplifier.connect(node);
      } else {
        lfoAmplifier.connect(node);
      }
    },
  };
}

function minMax(v: number, min: number = 0, max: number = 100): number {
  return Math.max(Math.min(v, max), min);
}

const OscillatorSquare: React.FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [x, setX] = useState(50);
  const [y, setY] = useState(50);

  const mGainRef = useRef<GainNode | null>(null);
  const mACtxRef = useRef<AudioContext | null>(null);
  const lfoRef = useRef<LFO | null>(null);

  const lowPassRef = useRef<BiquadFilterNode | null>(null);
  const lowPassFreqRef = useRef<number>(4000);

  const noiseNodeRef = useRef<AudioBufferSourceNode | null>(null);
  const noiseGainRef = useRef<GainNode | null>(null);

  const playKick = () => {
    const aCtx = mACtxRef.current;

    if (!aCtx || !mGainRef.current) {
      return;
    }

    const time = aCtx.currentTime;
    const tone = 64;
    const decay = 0.2;
    const volume = 5;

    const oscillator = aCtx.createOscillator();
    const gain = aCtx.createGain();

    oscillator.frequency.setValueAtTime(0, time);
    oscillator.frequency.setValueAtTime(tone, time + 0.01);
    oscillator.frequency.exponentialRampToValueAtTime(10, time + decay);

    gain.gain.setValueAtTime(0, time);
    gain.gain.setValueAtTime(volume, time + 0.01);
    gain.gain.exponentialRampToValueAtTime(0.001, time + decay);

    oscillator.connect(gain);
    gain.connect(aCtx.destination);

    oscillator.start(time);
    oscillator.stop(time + decay);
  };

  const createBrownianNoise = () => {
    const aCtx = mACtxRef.current;
    const bufferSize = 2 * aCtx.sampleRate;

    const noiseBuffer = aCtx.createBuffer(1, bufferSize, aCtx.sampleRate);
    const output = noiseBuffer.getChannelData(0);
    let lastOut = 0.0;

    for (let i = 0; i < bufferSize; i += 1) {
      const white = Math.random() * 2 - 1;
      output[i] = (lastOut + 0.02 * white) / 1.02;
      lastOut = output[i];
      output[i] *= 3.5;
    }

    return noiseBuffer;
  }

  useEffect(() => {
    if (!mGainRef.current || !mACtxRef.current) {
      return;
    }

    if (isPlaying) {
      mGainRef.current.gain.linearRampToValueAtTime(
        0.5,
        mACtxRef.current.currentTime + 0.1
      );
    } else {
      mGainRef.current.gain.linearRampToValueAtTime(
        0.0,
        mACtxRef.current.currentTime + 0.3
      );
    }
  }, [isPlaying]);

  useEffect(() => {
    if (lowPassRef.current) {
      lowPassRef.current.frequency.value = 20 + 170.0 * (100 - y);
      lowPassFreqRef.current = 4000 + 170.0 * (100 - y);
    }
  }, [y]);

  useEffect(() => {
    if (lfoRef.current) {
      // lfoRef.current.setFrequency(1 + Math.exp(((100 - x) / 100.0) * 10) / 480);
      lfoRef.current.setFrequency(1.5 + 10 * (x / 100));
      lfoRef.current.setAmplify(
        lowPassFreqRef.current -
        lowPassFreqRef.current * 0.9 * ((100 - x) / 100.0)
      );
    }
  }, [x]);

  useEffect(() => {
    if (noiseGainRef.current) {
      noiseGainRef.current.gain.value = (200 - x - y) / 800;
    }
  }, [x, y]);

  useEffect(() => {
    const aCtx = new AudioContext();
    mACtxRef.current = aCtx;

    const masterGain = aCtx.createGain();
    masterGain.connect(aCtx.destination);
    masterGain.gain.value = 0;
    mGainRef.current = masterGain;

    const lfo = createLFO(aCtx);
    lfo.setFrequency(1);
    lfo.setAmplify(lowPassFreqRef.current - 20);

    lfoRef.current = lfo;

    const oscillator = aCtx.createOscillator();
    oscillator.type = 'square';
    oscillator.frequency.value = 65.41;
    oscillator.detune.value = -12;
    const gain = aCtx.createGain();
    gain.gain.value = 0.5;

    const sawOscillator = aCtx.createOscillator();
    sawOscillator.type = 'sawtooth';
    sawOscillator.frequency.value = 65.41;
    sawOscillator.detune.value = 5;

    sawOscillator.connect(gain);

    const lowPass = aCtx.createBiquadFilter();
    lowPass.type = 'lowpass';
    lowPass.frequency.value = lowPassFreqRef.current;
    lfo.connect(lowPass.frequency);
    lowPass.Q.value = 8;
    lowPassRef.current = lowPass;

    const noiseBuffer = createBrownianNoise();
    const noiseNode = aCtx.createBufferSource();
    noiseNode.buffer = noiseBuffer;
    noiseNode.loop = true;
    noiseNode.start();

    const noiseGain = aCtx.createGain();
    noiseGain.gain.value = 0.0;
    noiseNode.connect(noiseGain);
    noiseGain.connect(masterGain);

    noiseGainRef.current = noiseGain;
    noiseNodeRef.current = noiseNode;

    oscillator.connect(gain);
    gain.connect(lowPass);
    lowPass.connect(masterGain);

    oscillator.start();
    sawOscillator.start();
  }, []);

  const onMouseDown = (e: MouseEvent<HTMLDivElement>) => {
    setIsPlaying(true);
    const boxRect = (e.target as HTMLDivElement).getBoundingClientRect();

    playKick();

    setX(minMax(((e.clientX - boxRect.x) / squareSize) * 100, 0, 100));
    setY(minMax(((e.clientY - boxRect.y) / squareSize) * 100, 0, 100));
  };

  const onMouseMove = (e: MouseEvent<HTMLDivElement>) => {
    if (!isPlaying) {
      return;
    }

    const boxRect = (e.target as HTMLDivElement).getBoundingClientRect();

    setX(
      minMax(((e.clientX - Math.floor(boxRect.x)) / squareSize) * 100, 0, 100)
    );
    setY(
      minMax(((e.clientY - Math.floor(boxRect.y)) / squareSize) * 100, 0, 100)
    );
  };

  const onMouseUp = (e: MouseEvent<HTMLDivElement>) => {
    setIsPlaying(false);

    const boxRect = (e.target as HTMLDivElement).getBoundingClientRect();

    setX(minMax(((e.clientX - boxRect.x) / squareSize) * 100, 0, 100));
    setY(minMax(((e.clientY - boxRect.y) / squareSize) * 100, 0, 100));
  };


  return (
    <div className={styles.oscillatorContainer}>
      <div
        onMouseUp={onMouseUp}
        onMouseDown={onMouseDown}
        onMouseMove={onMouseMove}
        className={styles.oscillatorSquare}
        style={{
          width: squareSize,
          height: squareSize,
          transform: `
            perspective(${squareSize}px)
            rotateX(${(x / 100) * 10 - 5}deg)
            rotateY(${(y / 100) * 10 - 5}deg)
          `,
        }}
      >
        <div className={styles.oscillatorSquareInner} style={{
          width: squareSize,
          height: squareSize,
          transform: `scale(${isPlaying ? 1.1 : 0})`
        }} />
      </div>
    </div>
  );
};

export default OscillatorSquare;
