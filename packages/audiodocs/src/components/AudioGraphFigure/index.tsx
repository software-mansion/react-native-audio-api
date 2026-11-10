import clsx from 'clsx';
import React, { FC, useId } from 'react';

import { measureCodeText } from '../svgText';
import styles from './styles.module.css';

type NodeKind = 'source' | 'effect' | 'analysis' | 'destination';

interface GraphNode {
  id: string;
  label: string;
  kind: NodeKind;
  /** Horizontal center in diagram units. */
  cx: number;
  /** Top edge in diagram units. */
  y: number;
}

interface GraphEdge {
  from: string;
  to: string;
  /** Where the arrow enters the target's bottom edge, as a fraction of its width. */
  entry?: number;
  /** Height of the horizontal run for edges that need to jog sideways. */
  elbowY?: number;
}

const VIEW_WIDTH = 1000;
const VIEW_HEIGHT = 640;
const NODE_HEIGHT = 60;
const MIN_NODE_WIDTH = 170;
const LABEL_FONT_SIZE = 16;
const LABEL_PADDING = 28;
const CORNER_RADIUS = 8;

const NODES: GraphNode[] = [
  { id: 'destination', label: 'AudioDestinationNode', kind: 'destination', cx: 500, y: 20 },
  { id: 'gainLeft', label: 'GainNode', kind: 'effect', cx: 300, y: 150 },
  { id: 'analyser', label: 'AnalyserNode', kind: 'analysis', cx: 720, y: 150 },
  { id: 'gainMiddle', label: 'GainNode', kind: 'effect', cx: 500, y: 250 },
  { id: 'biquad', label: 'BiquadFilterNode', kind: 'effect', cx: 160, y: 300 },
  { id: 'bufferRight', label: 'AudioBufferSourceNode', kind: 'source', cx: 860, y: 300 },
  { id: 'bufferLeft', label: 'AudioBufferSourceNode', kind: 'source', cx: 160, y: 420 },
  { id: 'oscillatorLeft', label: 'OscillatorNode', kind: 'source', cx: 280, y: 560 },
  { id: 'oscillatorMiddle', label: 'OscillatorNode', kind: 'source', cx: 500, y: 560 },
  { id: 'oscillatorRight', label: 'OscillatorNode', kind: 'source', cx: 720, y: 560 },
];

const EDGES: GraphEdge[] = [
  { from: 'bufferLeft', to: 'biquad' },
  { from: 'biquad', to: 'gainLeft', elbowY: 240 },
  { from: 'gainLeft', to: 'destination', entry: 0.22, elbowY: 110 },
  { from: 'gainMiddle', to: 'destination' },
  { from: 'analyser', to: 'destination', entry: 0.78, elbowY: 110 },
  { from: 'bufferRight', to: 'analyser', elbowY: 260 },
  { from: 'oscillatorLeft', to: 'gainMiddle', entry: 0.2, elbowY: 330 },
  { from: 'oscillatorMiddle', to: 'gainMiddle' },
  { from: 'oscillatorRight', to: 'gainMiddle', entry: 0.8, elbowY: 345 },
];

const LEGEND: Array<{ kind: NodeKind; label: string }> = [
  { kind: 'source', label: 'Source nodes' },
  { kind: 'effect', label: 'Effect nodes' },
  { kind: 'analysis', label: 'Analysis nodes' },
  { kind: 'destination', label: 'Destination node' },
];

const KIND_CLASS: Record<NodeKind, string> = {
  source: styles.source,
  effect: styles.effect,
  analysis: styles.analysis,
  destination: styles.destination,
};

function nodeWidth(label: string): number {
  return Math.max(MIN_NODE_WIDTH, Math.ceil(measureCodeText(label, LABEL_FONT_SIZE) + LABEL_PADDING));
}

function edgePath(from: GraphNode, to: GraphNode, edge: GraphEdge, widths: Map<string, number>): string {
  const startX = from.cx;
  const startY = from.y;
  const targetWidth = widths.get(to.id) ?? MIN_NODE_WIDTH;
  const endX = to.cx - targetWidth / 2 + targetWidth * (edge.entry ?? 0.5);
  const endY = to.y + NODE_HEIGHT;

  if (Math.abs(startX - endX) < 1) {
    return `M ${startX} ${startY} V ${endY}`;
  }
  const elbowY = edge.elbowY ?? (startY + endY) / 2;
  return `M ${startX} ${startY} V ${elbowY} H ${endX} V ${endY}`;
}

/**
 * A sample audio graph: sources at the bottom, the destination at the top, every connection an
 * arrow in signal direction, and each box colored by the kind of node it is.
 */
const AudioGraphFigure: FC = () => {
  const markerId = useId();

  const widths = new Map(NODES.map((node) => [node.id, nodeWidth(node.label)]));
  const nodesById = new Map(NODES.map((node) => [node.id, node]));

  return (
    <figure className={styles.figure}>
      <div className={styles.scroller}>
        <svg
          className={styles.svg}
          viewBox={`0 0 ${VIEW_WIDTH} ${VIEW_HEIGHT}`}
          role="img"
          aria-label="Example audio graph: two audio buffer sources and three oscillators feed filters, gains and an analyser that all end in the audio destination">
          <defs>
            <marker id={markerId} markerWidth="9" markerHeight="9" refX="8" refY="4.5" orient="auto">
              <path className={styles.arrowHead} d="M0 0L9 4.5L0 9z" />
            </marker>
          </defs>

          {EDGES.map((edge) => {
            const from = nodesById.get(edge.from);
            const to = nodesById.get(edge.to);
            if (!from || !to) {
              return null;
            }
            return (
              <path
                key={`${edge.from}-${edge.to}`}
                className={styles.edge}
                d={edgePath(from, to, edge, widths)}
                markerEnd={`url(#${markerId})`}
              />
            );
          })}

          {NODES.map((node) => {
            const width = widths.get(node.id) ?? MIN_NODE_WIDTH;
            return (
              <g key={node.id} transform={`translate(${node.cx - width / 2} ${node.y})`}>
                <rect className={clsx(styles.box, KIND_CLASS[node.kind])} width={width} height={NODE_HEIGHT} rx={CORNER_RADIUS} />
                <text className={styles.label} x={width / 2} y={NODE_HEIGHT / 2} textAnchor="middle" dominantBaseline="middle">
                  {node.label}
                </text>
              </g>
            );
          })}
        </svg>
      </div>
      <figcaption className={styles.legend}>
        {LEGEND.map((entry) => (
          <span key={entry.kind} className={styles.legendItem}>
            <span className={clsx(styles.swatch, KIND_CLASS[entry.kind])} />
            {entry.label}
          </span>
        ))}
      </figcaption>
    </figure>
  );
};

export default AudioGraphFigure;
