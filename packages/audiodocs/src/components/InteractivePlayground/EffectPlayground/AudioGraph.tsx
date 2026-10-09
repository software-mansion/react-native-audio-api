import clsx from 'clsx';
import React, { FC, KeyboardEvent, ReactNode, useId } from 'react';

import { measureCodeText } from '@site/src/components/svgText';
import useContainerWidth from '@site/src/hooks/useContainerWidth';

import styles from './AudioGraph.module.css';

export interface GraphNodeSpec {
  label: string;
  sublabel?: string;
}

export interface AudioGraphSpec {
  /** Nodes between the source and the destination, in signal order; bypass covers all of them. */
  effects: GraphNodeSpec[];
  /** Node fed from the first effect's output and back into its input (delay feedback). */
  feedback?: GraphNodeSpec;
  /** The untouched signal always reaches the destination next to the effect (delay, reverb). */
  dryPath?: boolean;
}

interface AudioGraphProps {
  spec: AudioGraphSpec;
  enabled: boolean;
  isPlaying: boolean;
  isLoading: boolean;
  onEffectClick: () => void;
  onSourceClick: () => void;
}

const SOURCE_LABEL = 'AudioBufferSourceNode';
const DESTINATION_LABEL = 'AudioDestinationNode';

const MIN_NODE_WIDTH = 176;
const NODE_HEIGHT = 58;
const NODE_GAP = 44;
const VERTICAL_NODE_GAP = 44;
const PADDING = 10;
const CORNER_RADIUS = 6;
const LOOP_GAP = 34;
const BYPASS_LANE = 30;
const MIN_FEEDBACK_NODE_WIDTH_VERTICAL = 132;
const ICON_SIZE = 16;
const TEXT_INSET = 14;
const LABEL_FONT_SIZE = 12;
const WIDTH_STEP = 8;

/** Below this fraction of the horizontal layout's width the row would shrink until labels turn unreadable. */
const VERTICAL_LAYOUT_RATIO = 0.82;

type Orientation = 'horizontal' | 'vertical';

interface Rect {
  x: number;
  y: number;
  width: number;
  height: number;
}

interface Segment {
  from: [number, number];
  to: [number, number];
}

interface GraphLayout {
  width: number;
  height: number;
  source: Rect;
  effects: Rect[];
  destination: Rect;
  feedback?: Rect;
  chainEdges: Segment[];
  feedbackEdges: Segment[];
  bypassPath: string;
  bypassLabel: { x: number; y: number; rotate: number };
}

interface NodeWidths {
  source: number;
  effects: number[];
  destination: number;
  feedback: number;
}

function nodeWidth(label: string, hasIcon: boolean, minimum = MIN_NODE_WIDTH): number {
  const textStart = TEXT_INSET + (hasIcon ? ICON_SIZE + 8 : 0);
  const needed = textStart + measureCodeText(label, LABEL_FONT_SIZE) + TEXT_INSET;
  return Math.max(minimum, Math.ceil(needed / WIDTH_STEP) * WIDTH_STEP);
}

function measureWidths(spec: AudioGraphSpec): NodeWidths {
  return {
    source: nodeWidth(SOURCE_LABEL, true),
    effects: spec.effects.map((effect) => nodeWidth(effect.label, false)),
    destination: nodeWidth(DESTINATION_LABEL, true),
    feedback: spec.feedback ? nodeWidth(spec.feedback.label, false, MIN_FEEDBACK_NODE_WIDTH_VERTICAL) : 0,
  };
}

function horizontalLayoutWidth(widths: NodeWidths): number {
  const boxes = widths.source + widths.effects.reduce((sum, w) => sum + w, 0) + widths.destination;
  return PADDING * 2 + boxes + NODE_GAP * (widths.effects.length + 1);
}

function layoutHorizontal(spec: AudioGraphSpec, widths: NodeWidths): GraphLayout {
  const rowTop = PADDING + (spec.feedback ? NODE_HEIGHT + LOOP_GAP : 0);
  const midY = rowTop + NODE_HEIGHT / 2;
  const laneY = rowTop + NODE_HEIGHT + BYPASS_LANE;

  let cursor = PADDING;
  const place = (width: number): Rect => {
    const rect = { x: cursor, y: rowTop, width, height: NODE_HEIGHT };
    cursor += width + NODE_GAP;
    return rect;
  };
  const source = place(widths.source);
  const effects = widths.effects.map(place);
  const destination = place(widths.destination);

  const row = [source, ...effects, destination];
  const chainEdges: Segment[] = row.slice(1).map((rect, index) => ({
    from: [row[index].x + row[index].width, midY],
    to: [rect.x, midY],
  }));

  const anchor = effects[0];
  const feedback = spec.feedback
    ? { x: anchor.x, y: PADDING, width: anchor.width, height: NODE_HEIGHT }
    : undefined;

  return {
    width: horizontalLayoutWidth(widths),
    height: laneY + PADDING,
    source,
    effects,
    destination,
    feedback,
    chainEdges,
    feedbackEdges: feedback
      ? [
          { from: [anchor.x + anchor.width * 0.7, anchor.y], to: [feedback.x + anchor.width * 0.7, feedback.y + NODE_HEIGHT] },
          { from: [feedback.x + anchor.width * 0.3, feedback.y + NODE_HEIGHT], to: [anchor.x + anchor.width * 0.3, anchor.y] },
        ]
      : [],
    bypassPath: `M ${source.x + source.width / 2} ${source.y + NODE_HEIGHT} V ${laneY} H ${destination.x + destination.width / 2} V ${destination.y + NODE_HEIGHT}`,
    bypassLabel: { x: (source.x + source.width / 2 + destination.x + destination.width / 2) / 2, y: laneY - 6, rotate: 0 },
  };
}

function layoutVertical(spec: AudioGraphSpec, widths: NodeWidths): GraphLayout {
  const columnWidth = Math.max(widths.source, widths.destination, ...widths.effects);
  const columnX = PADDING + BYPASS_LANE;
  const midX = columnX + columnWidth / 2;
  const laneX = PADDING;

  let cursor = PADDING;
  const place = (): Rect => {
    const rect = { x: columnX, y: cursor, width: columnWidth, height: NODE_HEIGHT };
    cursor += NODE_HEIGHT + VERTICAL_NODE_GAP;
    return rect;
  };
  const source = place();
  const effects = spec.effects.map(place);
  const destination = place();

  const column = [source, ...effects, destination];
  const chainEdges: Segment[] = column.slice(1).map((rect, index) => ({
    from: [midX, column[index].y + NODE_HEIGHT],
    to: [midX, rect.y],
  }));

  const anchor = effects[0];
  const feedback = spec.feedback
    ? { x: columnX + columnWidth + LOOP_GAP, y: anchor.y, width: widths.feedback, height: NODE_HEIGHT }
    : undefined;

  return {
    width: columnX + columnWidth + (feedback ? LOOP_GAP + feedback.width : 0) + PADDING,
    height: destination.y + NODE_HEIGHT + PADDING,
    source,
    effects,
    destination,
    feedback,
    chainEdges,
    feedbackEdges: feedback
      ? [
          { from: [anchor.x + columnWidth, anchor.y + NODE_HEIGHT * 0.3], to: [feedback.x, feedback.y + NODE_HEIGHT * 0.3] },
          { from: [feedback.x, feedback.y + NODE_HEIGHT * 0.7], to: [anchor.x + columnWidth, anchor.y + NODE_HEIGHT * 0.7] },
        ]
      : [],
    bypassPath: `M ${source.x} ${source.y + NODE_HEIGHT / 2} H ${laneX} V ${destination.y + NODE_HEIGHT / 2} H ${destination.x}`,
    bypassLabel: { x: laneX - 4, y: (source.y + destination.y + NODE_HEIGHT) / 2, rotate: -90 },
  };
}

const PlayIcon: FC = () => <path className={styles.icon} d="M3 2.5v11l10-5.5z" />;

const StopIcon: FC = () => <rect className={styles.icon} x="3" y="3" width="10" height="10" rx="1.5" />;

const LoadingIcon: FC = () => <circle className={styles.spinner} cx="8" cy="8" r="5.5" />;

const SpeakerIcon: FC = () => (
  <g>
    <path className={styles.icon} d="M2 5.5h3l4-3.5v12l-4-3.5H2z" />
    <path className={styles.iconStroke} d="M11.5 5.5a3.5 3.5 0 0 1 0 5M13.5 3.5a6 6 0 0 1 0 9" />
  </g>
);

interface GraphNodeProps {
  rect: Rect;
  label: string;
  sublabel?: string;
  icon?: ReactNode;
  className?: string;
  ariaLabel?: string;
  pressed?: boolean;
  onClick?: () => void;
}

const GraphNode: FC<GraphNodeProps> = ({ rect, label, sublabel, icon, className, ariaLabel, pressed, onClick }) => {
  const interactive = Boolean(onClick);
  const textX = TEXT_INSET + (icon ? ICON_SIZE + 8 : 0);
  const centerY = rect.height / 2;

  const handleKeyDown = (event: KeyboardEvent<SVGGElement>) => {
    if (!onClick || (event.key !== 'Enter' && event.key !== ' ')) {
      return;
    }
    event.preventDefault();
    onClick();
  };

  return (
    <g
      className={clsx(styles.node, interactive && styles.interactive, className)}
      transform={`translate(${rect.x} ${rect.y})`}
      role={interactive ? 'button' : undefined}
      tabIndex={interactive ? 0 : undefined}
      aria-label={ariaLabel}
      aria-pressed={pressed}
      onClick={onClick}
      onKeyDown={handleKeyDown}>
      <rect className={styles.box} width={rect.width} height={rect.height} rx={CORNER_RADIUS} />
      {icon && <g transform={`translate(${TEXT_INSET} ${centerY - ICON_SIZE / 2})`}>{icon}</g>}
      <text className={styles.label} x={textX} y={sublabel ? centerY - 7 : centerY} dominantBaseline="middle">
        {label}
      </text>
      {sublabel && (
        <text className={styles.sublabel} x={textX} y={centerY + 10} dominantBaseline="middle">
          {sublabel}
        </text>
      )}
    </g>
  );
};

const AudioGraph: FC<AudioGraphProps> = ({ spec, enabled, isPlaying, isLoading, onEffectClick, onSourceClick }) => {
  const [containerRef, containerWidth] = useContainerWidth<HTMLDivElement>();
  const markerId = useId();

  const widths = measureWidths(spec);
  const orientation: Orientation =
    containerWidth > 0 && containerWidth < horizontalLayoutWidth(widths) * VERTICAL_LAYOUT_RATIO
      ? 'vertical'
      : 'horizontal';
  const layout = orientation === 'vertical' ? layoutVertical(spec, widths) : layoutHorizontal(spec, widths);

  const arrow = `url(#${markerId}-arrow)`;
  const dimmedArrow = `url(#${markerId}-arrow-dimmed)`;
  const showBypass = spec.dryPath || !enabled;
  const bypassLabel = spec.dryPath ? 'dry signal' : 'bypass';

  const sourceIcon = isLoading ? <LoadingIcon /> : isPlaying ? <StopIcon /> : <PlayIcon />;
  const sourceSublabel = isLoading ? 'loading…' : isPlaying ? 'playing · click to stop' : 'click to play';

  const renderEdge = (edge: Segment, key: string) => (
    <line
      key={key}
      className={clsx(styles.edge, !enabled && styles.edgeDimmed)}
      x1={edge.from[0]}
      y1={edge.from[1]}
      x2={edge.to[0]}
      y2={edge.to[1]}
      markerEnd={enabled ? arrow : dimmedArrow}
    />
  );

  return (
    <div ref={containerRef} className={styles.container}>
      <svg
        className={styles.graph}
        viewBox={`0 0 ${layout.width} ${layout.height}`}
        width="100%"
        style={{ maxWidth: layout.width }}
        role="group"
        aria-label="Audio graph of the playground">
        <defs>
          <marker id={`${markerId}-arrow`} markerWidth="8" markerHeight="8" refX="7" refY="4" orient="auto">
            <path className={styles.arrowHead} d="M0 0L8 4L0 8z" />
          </marker>
          <marker id={`${markerId}-arrow-dimmed`} markerWidth="8" markerHeight="8" refX="7" refY="4" orient="auto">
            <path className={styles.arrowHeadDimmed} d="M0 0L8 4L0 8z" />
          </marker>
        </defs>

        {layout.chainEdges.map((edge, index) => renderEdge(edge, `chain-${index}`))}
        {layout.feedbackEdges.map((edge, index) => renderEdge(edge, `feedback-${index}`))}

        {showBypass && (
          <g>
            <path className={styles.edge} d={layout.bypassPath} markerEnd={arrow} />
            <text
              className={styles.edgeLabel}
              x={layout.bypassLabel.x}
              y={layout.bypassLabel.y}
              textAnchor="middle"
              transform={`rotate(${layout.bypassLabel.rotate} ${layout.bypassLabel.x} ${layout.bypassLabel.y})`}>
              {bypassLabel}
            </text>
          </g>
        )}

        <GraphNode
          rect={layout.source}
          label={SOURCE_LABEL}
          sublabel={sourceSublabel}
          icon={sourceIcon}
          ariaLabel={isPlaying ? 'Stop playback' : 'Start playback'}
          onClick={onSourceClick}
        />
        {spec.effects.map((effect, index) => (
          <GraphNode
            key={effect.label}
            rect={layout.effects[index]}
            label={effect.label}
            sublabel={enabled ? effect.sublabel : 'bypassed · click to enable'}
            className={enabled ? styles.effectActive : styles.effectBypassed}
            ariaLabel={`${enabled ? 'Bypass' : 'Enable'} ${effect.label}`}
            pressed={enabled}
            onClick={onEffectClick}
          />
        ))}
        {layout.feedback && spec.feedback && (
          <GraphNode
            rect={layout.feedback}
            label={spec.feedback.label}
            sublabel={spec.feedback.sublabel}
            className={enabled ? undefined : styles.effectBypassed}
          />
        )}
        <GraphNode rect={layout.destination} label={DESTINATION_LABEL} sublabel="speakers" icon={<SpeakerIcon />} />
      </svg>
    </div>
  );
};

export default AudioGraph;
