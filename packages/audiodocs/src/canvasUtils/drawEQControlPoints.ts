import { frequencies } from "../audio/Equalizer";
import AudioManager from "../audio/AudioManager";

const minFreq = frequencies[0].frequency;
const maxFreq = frequencies[frequencies.length - 1].frequency;
const logMin = Math.log10(minFreq);
const logMax = Math.log10(maxFreq);

export default function drawEQControlPoints(
  canvas: HTMLCanvasElement,
  ctx: CanvasRenderingContext2D,
  x: number,
  y: number,
  width: number,
  height: number,
  fillColor: string
) {
  const controlPointSize = 48;
  const halfSize = controlPointSize / 2;

  frequencies.forEach((freqConfig, index) => {
    if (AudioManager.equalizer && AudioManager.equalizer.bands[index]) {
      const band = AudioManager.equalizer.bands[index];
      const frequency = freqConfig.frequency;
      const gainValue = band.gain.value;

      const logFreq = Math.log10(frequency);
      const normalizedPos = (logFreq - logMin) / (logMax - logMin);
      let posX = x + (normalizedPos * width);

      if (index === 0) {
        posX += 96;
      } else if (index === 1) {
        posX += 66;
      } else if (index === 2) {
        posX = x + width / 2 + 59;
      } else if (index === 3) {
        posX -= 18;
      } else {
        posX -= 96;
      }

      const normalizedGain = gainValue / 12;
      const posY = y + height / 2 - (normalizedGain * height / 2);

      ctx.beginPath();
      ctx.arc(posX, posY, halfSize, 0, Math.PI * 2);
      ctx.fillStyle = fillColor;
      ctx.fill();
      ctx.strokeStyle = fillColor;
      ctx.lineWidth = 2;
      ctx.stroke();

      // Draw frequency label
      ctx.fillStyle = '#FFFFFF';
      ctx.font = '16px Aeonik';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';

      // Format frequency display
      const freqLabel = (index + 1).toString();

      ctx.fillText(freqLabel, posX, posY);
    }
  });
}
