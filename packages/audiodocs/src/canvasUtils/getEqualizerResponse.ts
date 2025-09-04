import AudioManager from "../audio/AudioManager";
import { Point } from "./types";

const frequencies = [32, 100, 1000, 10000, 16000];
const minFreq = frequencies[0];
const maxFreq = frequencies[frequencies.length - 1];
const logMin = Math.log10(minFreq);
const logMax = Math.log10(maxFreq);

interface ResponsePoint {
  frequency: number;
  magnitude: number;
  posX: number;
}

export default function getEqualizerResponse(x: number, maxWidth: number, y: number, maxHeight: number): Point[] {
  const numPoints = 200;
  const responsePoints: ResponsePoint[] = [];

  for (let i = 0; i < numPoints; i += 1) {
    const logFreq = logMin + (i / (numPoints - 1)) * (logMax - logMin);
    const frequency = Math.pow(10, logFreq);

    const singleFreq = new Float32Array([frequency]);
    const singleMag = new Float32Array(1);
    const singlePhase = new Float32Array(1);

    let totalMagnitude = 1;

    AudioManager.equalizer.bands.forEach(band => {
      band.getFrequencyResponse(singleFreq, singleMag, singlePhase);
      totalMagnitude *= singleMag[0];
    });

    let posX: number;
    if (i === 0) {
      posX = x;
    } else if (i === numPoints - 1) {
      posX = x + maxWidth + 3;
    } else {
      const normalizedPos = i / (numPoints - 1);
      posX = x + (normalizedPos * maxWidth);
    }

    responsePoints.push({
      frequency,
      magnitude: totalMagnitude,
      posX
    });
  }

  return responsePoints.map(point => ({
    x: point.posX,
    y: y + maxHeight / 2 - (Math.log10(point.magnitude) * 12 * maxHeight / 24)
  }));
}
