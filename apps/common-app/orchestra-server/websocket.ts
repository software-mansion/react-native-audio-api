// Minimal RFC 6455 WebSocket server over node:http — enough for small JSON
// text messages (no extensions, no fragmentation across frames). Written
// instead of depending on `ws` so the conductor runs with a bare `node`.
import { createHash } from 'node:crypto';
import { EventEmitter } from 'node:events';
import type { IncomingMessage } from 'node:http';
import type { Duplex } from 'node:stream';

const HANDSHAKE_GUID = '258EAFA5-E914-47DA-95CA-C5AB0DC85B11';

const OPCODE_TEXT = 0x1;
const OPCODE_CLOSE = 0x8;
const OPCODE_PING = 0x9;
const OPCODE_PONG = 0xa;

interface Frame {
  opcode: number;
  payload: Buffer;
  consumed: number;
}

export class WebSocketConnection extends EventEmitter<{
  message: [text: string];
  close: [];
}> {
  private readonly socket: Duplex;
  private open = true;
  private pending: Buffer = Buffer.alloc(0);

  constructor(socket: Duplex) {
    super();
    this.socket = socket;

    socket.on('data', (chunk: Buffer) => this.consume(chunk));
    socket.on('close', () => this.finish());
    socket.on('error', () => this.finish());
  }

  send(text: string): void {
    if (!this.open) {
      return;
    }
    this.socket.write(encodeFrame(OPCODE_TEXT, Buffer.from(text, 'utf8')));
  }

  close(): void {
    if (!this.open) {
      return;
    }
    this.socket.end(encodeFrame(OPCODE_CLOSE, Buffer.alloc(0)));
    this.finish();
  }

  private consume(chunk: Buffer): void {
    this.pending = Buffer.concat([this.pending, chunk]);

    for (;;) {
      const frame = decodeFrame(this.pending);
      if (!frame) {
        return;
      }
      this.pending = this.pending.subarray(frame.consumed);

      switch (frame.opcode) {
        case OPCODE_TEXT:
          this.emit('message', frame.payload.toString('utf8'));
          break;
        case OPCODE_PING:
          this.socket.write(encodeFrame(OPCODE_PONG, frame.payload));
          break;
        case OPCODE_CLOSE:
          this.close();
          return;
        default:
          break;
      }
    }
  }

  private finish(): void {
    if (!this.open) {
      return;
    }
    this.open = false;
    this.socket.destroy();
    this.emit('close');
  }
}

export function acceptUpgrade(
  request: IncomingMessage,
  socket: Duplex
): WebSocketConnection | null {
  const key = request.headers['sec-websocket-key'];
  if (!key || request.headers.upgrade?.toLowerCase() !== 'websocket') {
    socket.end('HTTP/1.1 400 Bad Request\r\n\r\n');
    return null;
  }

  const accept = createHash('sha1')
    .update(key + HANDSHAKE_GUID)
    .digest('base64');

  socket.write(
    [
      'HTTP/1.1 101 Switching Protocols',
      'Upgrade: websocket',
      'Connection: Upgrade',
      `Sec-WebSocket-Accept: ${accept}`,
      '',
      '',
    ].join('\r\n')
  );

  return new WebSocketConnection(socket);
}

function encodeFrame(opcode: number, payload: Buffer): Buffer {
  const length = payload.length;
  let header: Buffer;

  if (length < 126) {
    header = Buffer.from([0x80 | opcode, length]);
  } else if (length < 0x10000) {
    header = Buffer.alloc(4);
    header[0] = 0x80 | opcode;
    header[1] = 126;
    header.writeUInt16BE(length, 2);
  } else {
    header = Buffer.alloc(10);
    header[0] = 0x80 | opcode;
    header[1] = 127;
    header.writeBigUInt64BE(BigInt(length), 2);
  }

  return Buffer.concat([header, payload]);
}

function decodeFrame(buffer: Buffer): Frame | null {
  if (buffer.length < 2) {
    return null;
  }

  const opcode = buffer[0] & 0x0f;
  const masked = (buffer[1] & 0x80) !== 0;
  let length = buffer[1] & 0x7f;
  let offset = 2;

  if (length === 126) {
    if (buffer.length < 4) {
      return null;
    }
    length = buffer.readUInt16BE(2);
    offset = 4;
  } else if (length === 127) {
    if (buffer.length < 10) {
      return null;
    }
    length = Number(buffer.readBigUInt64BE(2));
    offset = 10;
  }

  const maskLength = masked ? 4 : 0;
  if (buffer.length < offset + maskLength + length) {
    return null;
  }

  const mask = masked ? buffer.subarray(offset, offset + 4) : null;
  const payload = Buffer.from(
    buffer.subarray(offset + maskLength, offset + maskLength + length)
  );

  if (mask) {
    for (let i = 0; i < payload.length; i += 1) {
      payload[i] ^= mask[i % 4];
    }
  }

  return { opcode, payload, consumed: offset + maskLength + length };
}
