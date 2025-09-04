
interface GridColumn {
  label?: string;
}

interface GridRow {
  label?: string;
}

interface GridOptions {
  columns: GridColumn[];
  rows: GridRow[];
}

const defaultOptions: GridOptions = {
  columns: [{}, { label: '100' }, { label: '1k' }, { label: '10k' },  {}],
  rows: [{}, { label: '+6' }, { label: '0' }, { label: '-6' }, {}],
};

// size of the whitespace under/after the grid to draw the labels
// TODO: better name?
const labelBoxSize = 32;

export function getDrawingBounds(canvas: HTMLCanvasElement): { x: number; y: number; width: number; height: number } {
  const { width, height } = canvas;

  return {
    x: 0,
    y: 0,
    width: width - labelBoxSize,
    height: height - labelBoxSize,
  };
}

function drawColumns(ctx: CanvasRenderingContext2D, columns: GridColumn[], x: number, y: number, width: number, height: number) {
  const columnWidth = width / (columns.length - 1);

  columns.forEach((_c, index) => {
    const colX = x + index * columnWidth;

    ctx.strokeStyle = '#C1C6E5';
    ctx.lineWidth = 0.5;
    ctx.beginPath();
    ctx.moveTo(colX, y);
    ctx.lineTo(colX, y + height);
    ctx.stroke();
  });
}

function drawRows(ctx: CanvasRenderingContext2D, rows: GridRow[], x: number, y: number, width: number, height: number) {
  const rowHeight = height / (rows.length - 1);

  rows.forEach((_r, index) => {
    const rowY = y + index * rowHeight;

    ctx.strokeStyle = '#C1C6E5';
    ctx.lineWidth = 0.5;
    ctx.beginPath();
    ctx.moveTo(x, rowY);
    ctx.lineTo(x + width, rowY);
    ctx.stroke();
  });
}

function drawColumnLabels(ctx: CanvasRenderingContext2D, columns: GridColumn[], x: number, y: number, width: number, height: number) {
  const columnWidth = width / (columns.length - 1);

  columns.forEach((column, index) => {
    if (!column.label) {
      return;
    }

    ctx.fillStyle = '#C1C6E5';
    ctx.font = '400 16px Aeonik';
    const textWidth = ctx.measureText(column.label).width;

    const colX = x + index * columnWidth;
    const textX = colX;
    ctx.fillText(column.label, textX, y + height + 20);
  });
}

function drawRowLabels(ctx: CanvasRenderingContext2D, rows: GridRow[], x: number, y: number, width: number, height: number) {
  const rowHeight = height / (rows.length - 1);

  rows.forEach((row, index) => {
    if (!row.label) {
      return;
    }

    ctx.fillStyle = '#C1C6E5';
    ctx.font = '400 16px Aeonik';
    const textMetrics = ctx.measureText(row.label);
    const textHeight = textMetrics.actualBoundingBoxAscent + textMetrics.actualBoundingBoxDescent;

    const rowY = y + index * rowHeight;
    ctx.fillText(row.label, width + 16, rowY);
  });
}

export default function drawEqGrid(canvas: HTMLCanvasElement, ctx: CanvasRenderingContext2D, options: GridOptions = defaultOptions) {
  const { columns, rows } = options;
  const { x, y, width, height } = getDrawingBounds(canvas);

  drawColumns(ctx, columns, x, y, width, height);
  drawRows(ctx, rows, x, y, width, height);

  drawColumnLabels(ctx, columns, x, y, width, height);
  drawRowLabels(ctx, rows, x, y, width, height);
};
