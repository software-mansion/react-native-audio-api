import React, { useRef, useState, useEffect, useCallback } from 'react';
import styles from './OscillatorSquare.module.css'

interface Point {
  x: number;
  y: number;
}

interface LFO {
  lfo: OscillatorNode;
  lfoAmplifier: GainNode;
  setFrequency: (value: number) => void;
  setAmplify: (value: number) => void;
  connect: (node: AudioNode | AudioParam) => void;
}

const squareSize = 220;

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

function clamp(v: number, min: number = 0, max: number = 100): number {
  return Math.max(Math.min(v, max), min);
}

const OscillatorSquare: React.FC = () => {
  const rectRef = useRef<HTMLDivElement | null>(null);
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

  const preventScroll = useCallback((e: Event) => {
    e.preventDefault();
  }, []);

  const playKick = () => {
    const aCtx = mACtxRef.current;

    if (!aCtx || !mGainRef.current) {
      return;
    }

    const time = aCtx.currentTime;
    const tone = 164;
    const decay = 0.2;
    const volume = 5;

    const oscillator = aCtx.createOscillator();
    const gain = aCtx.createGain();

    oscillator.frequency.setValueAtTime(0, time);
    oscillator.frequency.setValueAtTime(tone, time + 0.005);
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

  const enableScrollBlock = () => {
    window.addEventListener('touchmove', preventScroll as EventListener, { passive: false });
  };
  const disableScrollBlock = () => {
    window.removeEventListener('touchmove', preventScroll as EventListener);
  };

  const onStart = (pointer: Point) => {
    const box = rectRef.current?.getBoundingClientRect();

    playKick();
    setIsPlaying(true);

    setX(clamp(((pointer.x - box.x) / squareSize) * 100, 0, 100));
    setY(clamp(((pointer.y - box.y) / squareSize) * 100, 0, 100));

    enableScrollBlock();
  }

  const onMove = (pointer: Point) => {
    if (!isPlaying) {
      return;
    }

    const box = rectRef.current?.getBoundingClientRect();

    setX(
      clamp((Math.floor(Math.min(pointer.x, box.x + box.width) - box.x) / squareSize) * 100, 0, 100)
    );
    setY(
      clamp((Math.floor(Math.min(pointer.y, box.y + box.height) - box.y) / squareSize) * 100, 0, 100)
    );
  }

  const onStop = () => {
    setIsPlaying(false);
    disableScrollBlock();
  }

  const onMouseDown = (e: React.MouseEvent<HTMLDivElement>) => {
    e.preventDefault();

    onStart({ x: e.clientX, y: e.clientY });
  };

  const onMouseMove = (e: MouseEvent) => {
    e.preventDefault();

    onMove({ x: e.clientX, y: e.clientY });
  };

  const onTouchStart = (e: React.TouchEvent<HTMLDivElement>) => {
    onStart({ x: e.touches[0].clientX, y: e.touches[0].clientY });
  }

  const onTouchMove = (e: TouchEvent) => {
    onMove({ x: e.touches[0].clientX, y: e.touches[0].clientY });
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
      lowPassFreqRef.current = 4000 + 200.0 * (100 - y);
    }
  }, [y]);

  useEffect(() => {
    if (lfoRef.current) {
      // https://www.wolframalpha.com/input?i=2*e%5E%282.35*x%29+-+2%2C+x+%3D+0+to+1
      lfoRef.current.setFrequency(2 * Math.exp((x / 100.0) * 2.35) - 0.5);

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
    sawOscillator.frequency.value = 130.81;
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

  useEffect(() => {
    window.addEventListener('mouseup', onStop);
    window.addEventListener('touchend', onStop);
    window.addEventListener('mousemove', onMouseMove);
    window.addEventListener('touchmove', onTouchMove, {passive: false});

    return () => {
      window.removeEventListener('mouseup', onStop);
      window.removeEventListener('touchend', onStop);
      window.removeEventListener('mousemove', onMouseMove);
      window.removeEventListener('touchmove', onTouchMove);
    };
  }, [isPlaying]);

    useEffect(() => {
    return () => {
      disableScrollBlock();
    };
  }, [preventScroll]);

  return (
    <div className={styles.oscillatorContainer}>
      <div
        ref={rectRef}
        onMouseDown={onMouseDown}
        onTouchStart={onTouchStart}
        className={styles.oscillator}
        style={{
          width: squareSize,
          height: squareSize,
          touchAction: 'none',
          transform: `
            perspective(150px)
            rotateX(${(x / 100) * 10 - 5}deg)
            rotateY(${(y / 100) * 10 - 5}deg)
          `,
        }}
      >
        <div className={styles.oscillatorBorder} />
        <div
          className={styles.oscillatorFill}
          style={{
            transform: `scale(${isPlaying ? 1 : 0})`,
            opacity: isPlaying ? 1 : 0,
          }}
        />
        <div
          className={styles.oscillatorPointer}
          style={{
            top: `${(clamp(y / 100 * squareSize, 12, squareSize - 12))}px`,
            left: `${(clamp(x / 100 * squareSize, 12, squareSize - 12))}px`,
          }}
        >
          <div
            className={isPlaying ? styles.oscillatorPointerInnerOut : styles.oscillatorPointerInnerIn}
          />
        </div>
      </div>
    </div>
  );
};

export default OscillatorSquare;