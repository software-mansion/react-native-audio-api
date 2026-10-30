// Renders a QR code as a standalone SVG, for the dashboard's join code.
import { Ecc, QrCode } from './qrcodegen.ts';

const QUIET_ZONE_MODULES = 4;

export function qrSvg(text: string): string {
  const qr = QrCode.encodeText(text, Ecc.MEDIUM);
  const span = qr.size + QUIET_ZONE_MODULES * 2;

  const squares: string[] = [];
  for (let y = 0; y < qr.size; y += 1) {
    for (let x = 0; x < qr.size; x += 1) {
      if (qr.getModule(x, y)) {
        squares.push(
          `M${x + QUIET_ZONE_MODULES},${y + QUIET_ZONE_MODULES}h1v1h-1z`
        );
      }
    }
  }

  return (
    `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${span} ${span}" shape-rendering="crispEdges">` +
    `<rect width="${span}" height="${span}" fill="#fff"/>` +
    `<path d="${squares.join('')}" fill="#000"/>` +
    '</svg>'
  );
}
