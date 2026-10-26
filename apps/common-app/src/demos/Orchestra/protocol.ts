// Wire format shared by the phone app and `orchestra-server/`. The server
// imports this file type-only, so keep it free of runtime code.
//
// Every time value is milliseconds on the server's monotonic clock unless the
// field name says otherwise. `transport.startAt` is the server time at which
// beat `transport.startBeat` sounds.

export type Timbre =
  | 'bell'
  | 'pluck'
  | 'pad'
  | 'lead'
  | 'hat'
  | 'clap'
  /**
   * Drums: `midi` is a General MIDI percussion key (see orchestra-server/drumKit.ts), played on
   * the recorded kit or, as a stand-in, by the hat and clap voices.
   */
  | 'kit'
  | 'click';

export interface ScoreNote {
  beat: number;
  midi: number;
  /** In beats. */
  duration: number;
  /** 0..1 */
  velocity: number;
}

export interface ScorePart {
  name: string;
  color: string;
  timbre: Timbre;
  notes: ScoreNote[];
  /**
   * A recorded instrument to play the part with: a General MIDI name from
   * FluidR3_GM (e.g. `violin`), served by the orchestra server under
   * `/samples`. Phones use `timbre` until its samples have loaded, or if the
   * server has none.
   */
  instrument?: string;
  /**
   * Phones dealt this part automatically; unlimited when absent. The conductor
   * can still assign more by hand.
   */
  maxPlayers?: number;
}

export interface Score {
  id: string;
  title: string;
  bpm: number;
  lengthBeats: number;
  /** From the piece's time signature; 4 when absent. */
  beatsPerBar?: number;
  parts: ScorePart[];
}

export interface Transport {
  playing: boolean;
  startAt: number;
  /**
   * The score beat that sounds at `startAt`: playback can begin mid-piece.
   * Absent from servers that predate it, which always start at 0.
   */
  startBeat?: number;
  bpm: number;
  /** Incremented on every start so phones reset their note cursor. */
  epoch: number;
}

export interface PlayerInfo {
  id: number;
  name?: string;
  platform?: string;
  /**
   * Parts this phone plays, as indexes into `score.parts`; may be several, or
   * none.
   */
  partIndices: number[];
  /** Set by the conductor: the phone keeps time but plays nothing. */
  muted?: boolean;
  offsetMs?: number;
  rttMs?: number;
  uncertaintyMs?: number;
  jitterMs?: number;
  outputLatencyMs?: number;
  trimMs?: number;
  joinedAt?: number;
  reportedAt?: number;
}

export type ClientMessage =
  | {
      type: 'hello';
      role: 'player';
      name: string;
      platform: string;
      outputLatencyMs: number;
    }
  | { type: 'hello'; role: 'conductor' }
  | { type: 'ping'; t0: number }
  | {
      type: 'report';
      offsetMs: number;
      rttMs: number;
      uncertaintyMs: number;
      jitterMs: number;
      outputLatencyMs: number;
      trimMs: number;
    }
  /**
   * `startBar` counts from 1; playback starts at the top of the first bar when
   * absent.
   */
  | { type: 'start'; inMs: number; startBar?: number }
  | { type: 'stop' }
  | { type: 'setScore'; scoreId: string }
  | { type: 'setBpm'; bpm: number }
  | { type: 'assignParts'; playerId: number; partIndices: number[] }
  | { type: 'setMuted'; playerId: number; muted: boolean }
  | { type: 'rebalance' };

export type ServerMessage =
  | { type: 'welcome'; playerId: number; serverTime: number }
  | { type: 'pong'; t0: number; t1: number; t2: number }
  | {
      type: 'state';
      serverTime: number;
      transport: Transport;
      score: Score;
      players: PlayerInfo[];
      /** Every score the conductor can switch to. */
      scores: { id: string; title: string }[];
    };
