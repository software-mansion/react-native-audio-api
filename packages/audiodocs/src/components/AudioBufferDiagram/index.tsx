import React, { FC } from 'react';

import useContainerWidth from '@site/src/hooks/useContainerWidth';

import styles from './styles.module.css';

interface AudioBufferDiagramProps {
  /** Channel names, top to bottom. */
  channels?: string[];
  /** Upper bound on the frames drawn; fewer are drawn when the container is narrower. */
  maxFrames?: number;
}

const DEFAULT_CHANNELS = ['Left', 'Right'];
const DEFAULT_MAX_FRAMES = 8;
const MIN_FRAMES = 3;

const PADDING = 8;
const LABEL_COLUMN_WIDTH = 128;
const CELL_WIDTH = 72;
const CELL_HEIGHT = 40;
const HEADER_HEIGHT = 30;
const COLUMN_GAP = 6;
const ELLIPSIS_WIDTH = 30;
const BAND_INSET = 4;
const BRACKET_GAP = 12;
const BRACKET_TICK = 6;
const BRACKET_LABEL_GAP = 16;
const BRACKET_LABEL_LINE_HEIGHT = 16;
const CORNER_RADIUS = 10;

function framesThatFit(containerWidth: number, maxFrames: number): number {
  if (containerWidth <= 0) {
    return maxFrames;
  }
  const available = containerWidth - PADDING * 2 - LABEL_COLUMN_WIDTH - COLUMN_GAP - ELLIPSIS_WIDTH;
  const fitting = Math.floor((available + COLUMN_GAP) / (CELL_WIDTH + COLUMN_GAP));
  return Math.min(maxFrames, Math.max(MIN_FRAMES, fitting));
}

/**
 * Frames-by-channels grid of an AudioBuffer: every column is one frame, every row one channel,
 * and each cell holds a single sample.
 */
const AudioBufferDiagram: FC<AudioBufferDiagramProps> = ({
  channels = DEFAULT_CHANNELS,
  maxFrames = DEFAULT_MAX_FRAMES,
}) => {
  const [containerRef, containerWidth] = useContainerWidth<HTMLDivElement>();
  const frames = framesThatFit(containerWidth, maxFrames);

  const gridLeft = PADDING + LABEL_COLUMN_WIDTH;
  const gridTop = PADDING;
  const columnStride = CELL_WIDTH + COLUMN_GAP;
  const gridRight = gridLeft + frames * columnStride - COLUMN_GAP;
  const bandTop = gridTop + HEADER_HEIGHT;
  const bandBottom = bandTop + channels.length * CELL_HEIGHT;
  const bracketY = bandBottom + BRACKET_GAP;
  const width = gridRight + COLUMN_GAP + ELLIPSIS_WIDTH + PADDING;
  const bracketLabelY = bracketY + BRACKET_TICK + BRACKET_LABEL_GAP;
  const height = bracketLabelY + BRACKET_LABEL_LINE_HEIGHT + PADDING;

  return (
    <div ref={containerRef} className={styles.container}>
      <svg
        className={styles.diagram}
        viewBox={`0 0 ${width} ${height}`}
        width="100%"
        style={{ maxWidth: width }}
        role="img"
        aria-label={`An AudioBuffer as a grid: ${channels.length} channels, one row each, and one column per frame holding one sample per channel`}>
        {Array.from({ length: frames }, (_, index) => {
          const x = gridLeft + index * columnStride;
          return (
            <g key={index}>
              <rect
                className={styles.frameColumn}
                x={x}
                y={gridTop}
                width={CELL_WIDTH}
                height={HEADER_HEIGHT + channels.length * CELL_HEIGHT}
                rx={CORNER_RADIUS}
              />
              <text
                className={styles.frameHeader}
                x={x + CELL_WIDTH / 2}
                y={gridTop + HEADER_HEIGHT / 2}
                textAnchor="middle"
                dominantBaseline="middle">
                frame
              </text>
            </g>
          );
        })}

        <rect
          className={styles.channelBand}
          x={gridLeft - BAND_INSET}
          y={bandTop}
          width={gridRight - gridLeft + BAND_INSET * 2}
          height={bandBottom - bandTop}
          rx={3}
        />

        {channels.map((channel, row) => {
          const rowTop = bandTop + row * CELL_HEIGHT;
          const rowCenter = rowTop + CELL_HEIGHT / 2;
          return (
            <g key={channel}>
              {row > 0 && (
                <line
                  className={styles.channelSeparator}
                  x1={gridLeft - BAND_INSET}
                  y1={rowTop}
                  x2={gridRight + BAND_INSET}
                  y2={rowTop}
                />
              )}
              <text
                className={styles.channelLabel}
                x={gridLeft - BAND_INSET - 24}
                y={rowCenter}
                textAnchor="end"
                dominantBaseline="middle">
                {channel} channel
              </text>
              <text
                className={styles.channelArrow}
                x={gridLeft - BAND_INSET - 8}
                y={rowCenter}
                textAnchor="end"
                dominantBaseline="middle">
                →
              </text>
              {Array.from({ length: frames }, (_, index) => (
                <text
                  key={index}
                  className={styles.sample}
                  x={gridLeft + index * columnStride + CELL_WIDTH / 2}
                  y={rowCenter}
                  textAnchor="middle"
                  dominantBaseline="middle">
                  sample
                </text>
              ))}
              <text
                className={styles.ellipsis}
                x={gridRight + COLUMN_GAP + ELLIPSIS_WIDTH / 2}
                y={rowCenter}
                textAnchor="middle"
                dominantBaseline="middle">
                ⋯
              </text>
            </g>
          );
        })}

        <path
          className={styles.bracket}
          d={`M ${gridLeft} ${bracketY} v ${BRACKET_TICK} H ${gridRight + COLUMN_GAP + ELLIPSIS_WIDTH} v ${-BRACKET_TICK}`}
        />
        <text
          className={styles.bracketLabel}
          x={(gridLeft + gridRight + COLUMN_GAP + ELLIPSIS_WIDTH) / 2}
          y={bracketLabelY}
          textAnchor="middle"
          dominantBaseline="middle">
          <tspan>length = number of frames</tspan>
          <tspan x={(gridLeft + gridRight + COLUMN_GAP + ELLIPSIS_WIDTH) / 2} dy={BRACKET_LABEL_LINE_HEIGHT}>
            sampleRate = frames played per second
          </tspan>
        </text>
      </svg>
    </div>
  );
};

export default AudioBufferDiagram;
