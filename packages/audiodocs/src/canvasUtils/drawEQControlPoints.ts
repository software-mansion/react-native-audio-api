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

  // Draw control points for each equalizer band
  frequencies.forEach((freqConfig, index) => {
    if (AudioManager.equalizer && AudioManager.equalizer.bands[index]) {
      const band = AudioManager.equalizer.bands[index];
      const frequency = freqConfig.frequency;

      // Get current gain value in dB
      const gainValue = band.gain.value;

      // Calculate X position based on frequency (logarithmic scale)
      const logFreq = Math.log10(frequency);
      const normalizedPos = (logFreq - logMin) / (logMax - logMin);
      let posX = x + (normalizedPos * width);

      // Apply custom positioning:
      // Points 1,2,5 keep their current positions, point 3 goes to center + 96px, point 4 reflects point 2
      if (index === 0) {
        posX += 96; // Point 1 (index 0) - keep current position
      } else if (index === 1) {
        posX += 66; // Point 2 (index 1) - keep current position
      } else if (index === 2) {
        posX = x + width / 2 + 59; // Point 3 (index 2) - center + 86px to the right
      } else if (index === 3) {
        // Point 4 (index 3) - reflect point 2's position
        posX -= 18; // Reflect point 2's position
      } else {
        posX -= 96; // Point 5 (index 4) - keep current position
      }

      // Calculate Y position based on gain value
      // Assuming ±12dB range for display
      const normalizedGain = gainValue / 12; // -1 to 1 range
      const posY = y + height / 2 - (normalizedGain * height / 2);

      // Draw outer circle (border)
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
