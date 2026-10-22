// Phone orchestra server: one WebSocket server every phone syncs its clock
// against and takes the score + transport from. Run with `node server.ts`
// (Node 22.18+, no dependencies) and open http://<lan-ip>:8765 for the
// dashboard. The wire format is documented in `src/demos/Orchestra/protocol.ts`.
import { createServer } from 'node:http';
import { readFileSync } from 'node:fs';
import { networkInterfaces } from 'node:os';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';

import { acceptUpgrade, type WebSocketConnection } from './websocket.ts';
import { scores, sampleStore, DEFAULT_SCORE_ID } from './score.ts';
import { qrSvg } from './qr.ts';
import type {
  ClientMessage,
  PlayerInfo,
  Score,
  ServerMessage,
  Transport,
} from '../src/demos/Orchestra/protocol.ts';

const PORT = Number(process.env.PORT ?? 8765);
const CONDUCTOR_REFRESH_MS = 1000;
const MIN_COUNT_IN_MS = 500;

interface Client extends PlayerInfo {
  role: 'player' | 'conductor';
  connection: WebSocketConnection;
}

const here = dirname(fileURLToPath(import.meta.url));
const dashboardHtml = readFileSync(join(here, 'dashboard.html'));

const serverNow = (): number => performance.now();

const transport: Transport = {
  playing: false,
  startAt: 0,
  bpm: scores[DEFAULT_SCORE_ID].bpm,
  epoch: 0,
};
let scoreId = DEFAULT_SCORE_ID;

const clients = new Map<number, Client>();
let nextClientId = 1;

function currentScore(): Score {
  return scores[scoreId];
}

function players(): Client[] {
  return [...clients.values()].filter((client) => client.role === 'player');
}

function playersSnapshot(): PlayerInfo[] {
  return players().map(
    ({ connection: _connection, role: _role, ...info }) => info
  );
}

function stateMessage(): string {
  const message: ServerMessage = {
    type: 'state',
    serverTime: serverNow(),
    transport,
    score: currentScore(),
    players: playersSnapshot(),
    scores: Object.values(scores).map(({ id, title }) => ({ id, title })),
  };
  return JSON.stringify(message);
}

function send(client: Client, message: ServerMessage): void {
  client.connection.send(JSON.stringify(message));
}

function broadcastState(): void {
  const message = stateMessage();
  for (const client of clients.values()) {
    client.connection.send(message);
  }
}

function sendToConductors(message: string): void {
  for (const client of clients.values()) {
    if (client.role === 'conductor') {
      client.connection.send(message);
    }
  }
}

// The emptiest part that still has room, earliest part on ties, so every part
// gets a phone before any part gets two, and a part capped at `maxPlayers` is
// skipped once full. If every part is full, the cap is ignored.
function leastPopulatedPart(counts: number[]): number {
  const { parts } = currentScore();
  const hasRoom = (index: number) =>
    counts[index] < (parts[index].maxPlayers ?? Number.POSITIVE_INFINITY);

  let best = -1;
  for (let index = 0; index < parts.length; index += 1) {
    if (hasRoom(index) && (best < 0 || counts[index] < counts[best])) {
      best = index;
    }
  }
  return best >= 0 ? best : counts.indexOf(Math.min(...counts));
}

function partCounts(except?: Client): number[] {
  const counts = new Array<number>(currentScore().parts.length).fill(0);
  for (const player of players()) {
    if (player !== except) {
      for (const index of player.partIndices) {
        counts[index] += 1;
      }
    }
  }
  return counts;
}

function rebalanceParts(): void {
  const counts = new Array<number>(currentScore().parts.length).fill(0);
  for (const player of players()) {
    const part = leastPopulatedPart(counts);
    player.partIndices = [part];
    counts[part] += 1;
  }
}

// Changing tempo mid-piece keeps the current beat position so phones do not
// jump back to the start.
function setBpm(bpm: number): void {
  if (!(bpm > 20 && bpm < 400)) {
    return;
  }
  if (transport.playing) {
    const now = serverNow();
    const beatNow = ((now - transport.startAt) * transport.bpm) / 60000;
    transport.startAt = now - (beatNow * 60000) / bpm;
  }
  transport.bpm = bpm;
}

function handleMessage(client: Client, raw: string): void {
  let message: ClientMessage;
  try {
    message = JSON.parse(raw) as ClientMessage;
  } catch {
    return;
  }

  switch (message.type) {
    case 'ping': {
      const t1 = serverNow();
      send(client, { type: 'pong', t0: message.t0, t1, t2: serverNow() });
      return;
    }
    case 'hello': {
      client.role = message.role;
      if (message.role === 'player') {
        client.name = message.name.slice(0, 24);
        client.platform = message.platform;
        client.outputLatencyMs = message.outputLatencyMs;
        client.partIndices = [leastPopulatedPart(partCounts(client))];
        client.joinedAt = serverNow();
        console.log(
          `+ ${client.name} (${client.platform}) → part ${client.partIndices[0]}`
        );
      }
      send(client, {
        type: 'welcome',
        playerId: client.id,
        serverTime: serverNow(),
      });
      broadcastState();
      return;
    }
    case 'report': {
      client.offsetMs = message.offsetMs;
      client.rttMs = message.rttMs;
      client.uncertaintyMs = message.uncertaintyMs;
      client.jitterMs = message.jitterMs;
      client.outputLatencyMs = message.outputLatencyMs;
      client.trimMs = message.trimMs;
      client.reportedAt = serverNow();
      return;
    }
    default:
      break;
  }

  if (client.role !== 'conductor') {
    return;
  }

  switch (message.type) {
    case 'start':
      transport.playing = true;
      transport.startAt = serverNow() + Math.max(MIN_COUNT_IN_MS, message.inMs);
      transport.epoch += 1;
      console.log(
        `▶ start in ${(transport.startAt - serverNow()).toFixed(0)} ms, ${transport.bpm} bpm, ${scoreId}`
      );
      break;
    case 'stop':
      transport.playing = false;
      console.log('■ stop');
      break;
    case 'setScore':
      if (scores[message.scoreId] && message.scoreId !== scoreId) {
        scoreId = message.scoreId;
        transport.playing = false;
        transport.bpm = currentScore().bpm;
        rebalanceParts();
      }
      break;
    case 'setBpm':
      setBpm(message.bpm);
      break;
    case 'assignParts': {
      const target = clients.get(message.playerId);
      const partCount = currentScore().parts.length;
      const valid = message.partIndices.filter(
        (index) => Number.isInteger(index) && index >= 0 && index < partCount
      );
      if (target?.role === 'player') {
        target.partIndices = [...new Set(valid)].sort((a, b) => a - b);
      }
      break;
    }
    case 'setMuted': {
      const target = clients.get(message.playerId);
      if (target?.role === 'player') {
        target.muted = message.muted;
      }
      break;
    }
    case 'rebalance':
      rebalanceParts();
      break;
    default:
      return;
  }

  broadcastState();
}

const httpServer = createServer((request, response) => {
  const url = new URL(request.url ?? '/', 'http://localhost');

  // The dashboard's join code: the phone's camera opens the app on the
  // Orchestra screen with the server URL filled in.
  if (url.pathname === '/qr.svg') {
    const data = url.searchParams.get('data') ?? '';
    response.writeHead(200, {
      'Content-Type': 'image/svg+xml',
      'Cache-Control': 'no-store',
    });
    response.end(qrSvg(data));
    return;
  }
  const drum = url.pathname.match(
    /^\/samples\/drums\/([a-zA-Z]+)\/(\d+)\.wav$/
  );
  if (drum) {
    sampleStore.getDrum(drum[1], Number(drum[2])).then((body) => {
      if (!body) {
        response.writeHead(404);
        response.end();
        return;
      }
      response.writeHead(200, {
        'Content-Type': 'audio/wav',
        'Cache-Control': 'public, max-age=86400',
      });
      response.end(body);
    });
    return;
  }
  // Phones load the notes of their recorded instrument from here.
  const sample = url.pathname.match(/^\/samples\/([^/]+)\/([^/]+)\.mp3$/);
  if (sample) {
    sampleStore.get(sample[1], sample[2]).then((body) => {
      if (!body) {
        response.writeHead(404);
        response.end();
        return;
      }
      response.writeHead(200, {
        'Content-Type': 'audio/mpeg',
        'Cache-Control': 'public, max-age=86400',
      });
      response.end(body);
    });
    return;
  }
  if (url.pathname === '/health') {
    response.writeHead(200, { 'Content-Type': 'application/json' });
    response.end(
      JSON.stringify({ players: players().length, transport, scoreId })
    );
    return;
  }
  response.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8' });
  response.end(dashboardHtml);
});

httpServer.on('upgrade', (request, socket) => {
  const connection = acceptUpgrade(request, socket);
  if (!connection) {
    return;
  }

  const client: Client = {
    id: nextClientId++,
    role: 'player',
    connection,
    partIndices: [0],
  };
  clients.set(client.id, client);

  connection.on('message', (raw) => handleMessage(client, raw));
  connection.on('close', () => {
    clients.delete(client.id);
    if (client.name) {
      console.log(`- ${client.name}`);
    }
    broadcastState();
  });
});

setInterval(() => sendToConductors(stateMessage()), CONDUCTOR_REFRESH_MS);

httpServer.listen(PORT, '0.0.0.0', () => {
  const addresses = Object.values(networkInterfaces())
    .flat()
    .filter((iface) => iface?.family === 'IPv4' && !iface.internal)
    .map((iface) => iface!.address);

  console.log('Phone orchestra');
  for (const address of addresses) {
    console.log(`  phones:    ws://${address}:${PORT}`);
    console.log(`  dashboard: http://${address}:${PORT}`);
  }
  void sampleStore.prefetch(Object.values(scores));
});
