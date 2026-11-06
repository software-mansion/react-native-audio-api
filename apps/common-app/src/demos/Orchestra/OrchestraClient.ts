// WebSocket link to the conductor: keeps the connection alive, feeds
// ClockSync with ping/pong samples (a burst right after connecting, then a
// slow trickle), reports sync quality back, and publishes the latest server
// state to the screen.
import ClockSync from './ClockSync';
import type { ClientMessage, PlayerInfo, ServerMessage } from './protocol';

type StateMessage = Extract<ServerMessage, { type: 'state' }>;

export type ConnectionStatus = 'disconnected' | 'connecting' | 'connected';

export interface OrchestraSnapshot {
  status: ConnectionStatus;
  playerId: number | null;
  state: StateMessage | null;
  me: PlayerInfo | null;
}

export interface LocalReport {
  outputLatencyMs: number;
  trimMs: number;
}

const INITIAL_PING_BURST = 24;
const BURST_INTERVAL_MS = 60;
const STEADY_PING_INTERVAL_MS = 1000;
const REPORT_INTERVAL_MS = 2000;
const RECONNECT_DELAY_MS = 1500;

export default class OrchestraClient {
  readonly clockSync = new ClockSync();

  private readonly url: string;
  private readonly name: string;
  private readonly platform: string;
  private readonly readLocal: () => LocalReport;
  private readonly onChange: (snapshot: OrchestraSnapshot) => void;

  private socket: WebSocket | null = null;
  private wanted = false;
  private snapshot: OrchestraSnapshot = {
    status: 'disconnected',
    playerId: null,
    state: null,
    me: null,
  };
  private timers: ReturnType<typeof setTimeout>[] = [];

  constructor(options: {
    url: string;
    name: string;
    platform: string;
    readLocal: () => LocalReport;
    onChange: (snapshot: OrchestraSnapshot) => void;
  }) {
    this.url = options.url;
    this.name = options.name;
    this.platform = options.platform;
    this.readLocal = options.readLocal;
    this.onChange = options.onChange;
  }

  connect(): void {
    this.wanted = true;
    this.open();
  }

  disconnect(): void {
    this.wanted = false;
    this.clearTimers();
    this.socket?.close();
    this.socket = null;
    this.publish({
      status: 'disconnected',
      playerId: null,
      state: null,
      me: null,
    });
  }

  /** Starts the piece on every phone after `inMs` of count-in. */
  start(inMs: number): void {
    this.send({ type: 'start', inMs });
  }

  stop(): void {
    this.send({ type: 'stop' });
  }

  private open(): void {
    this.publish({ status: 'connecting' });

    const socket = new WebSocket(this.url);
    this.socket = socket;

    socket.onopen = () => {
      this.publish({ status: 'connected' });
      this.send({
        type: 'hello',
        role: 'player',
        name: this.name,
        platform: this.platform,
        outputLatencyMs: this.readLocal().outputLatencyMs,
      });
      this.startPinging();
      this.schedule(() => this.report(), REPORT_INTERVAL_MS);
    };

    socket.onmessage = (event: WebSocketMessageEvent) => {
      this.receive(JSON.parse(String(event.data)) as ServerMessage);
    };

    socket.onclose = () => {
      if (this.socket !== socket) {
        return;
      }
      this.socket = null;
      this.clearTimers();
      this.publish({
        status: 'disconnected',
        playerId: null,
        state: null,
        me: null,
      });
      if (this.wanted) {
        this.schedule(() => this.open(), RECONNECT_DELAY_MS);
      }
    };

    socket.onerror = () => {
      // onclose follows and handles the retry.
    };
  }

  private receive(message: ServerMessage): void {
    switch (message.type) {
      case 'pong':
        this.clockSync.acceptPong(message);
        break;
      case 'welcome':
        this.publish({ playerId: message.playerId });
        break;
      case 'state':
        this.publish({
          state: message,
          me:
            message.players.find((p) => p.id === this.snapshot.playerId) ??
            null,
        });
        break;
    }
  }

  private startPinging(): void {
    let sent = 0;
    const burst = () => {
      this.send(this.clockSync.makePing());
      sent += 1;
      if (sent < INITIAL_PING_BURST) {
        this.schedule(burst, BURST_INTERVAL_MS);
      } else {
        this.schedule(steady, STEADY_PING_INTERVAL_MS);
      }
    };
    const steady = () => {
      this.send(this.clockSync.makePing());
      this.schedule(steady, STEADY_PING_INTERVAL_MS);
    };
    burst();
  }

  private report(): void {
    const { offsetMs, rttMs, uncertaintyMs, jitterMs } = this.clockSync.quality;
    const local = this.readLocal();
    this.send({
      type: 'report',
      offsetMs,
      rttMs,
      uncertaintyMs,
      jitterMs,
      outputLatencyMs: local.outputLatencyMs,
      trimMs: local.trimMs,
    });
    this.schedule(() => this.report(), REPORT_INTERVAL_MS);
  }

  private send(message: ClientMessage): void {
    if (this.socket?.readyState === WebSocket.OPEN) {
      this.socket.send(JSON.stringify(message));
    }
  }

  private schedule(fn: () => void, delayMs: number): void {
    const timer = setTimeout(() => {
      this.timers = this.timers.filter((t) => t !== timer);
      fn();
    }, delayMs);
    this.timers.push(timer);
  }

  private clearTimers(): void {
    this.timers.forEach(clearTimeout);
    this.timers = [];
  }

  private publish(patch: Partial<OrchestraSnapshot>): void {
    this.snapshot = { ...this.snapshot, ...patch };
    this.onChange(this.snapshot);
  }
}
