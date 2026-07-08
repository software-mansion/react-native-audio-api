type Float32ArrayViewFactory = (
  buffer: ArrayBufferLike,
  byteOffset: number,
  length: number
) => Float32Array;

let float32ArrayViewFactory: Float32ArrayViewFactory | undefined;

export function setFloat32ArrayViewFactory(
  factory: Float32ArrayViewFactory
): void {
  float32ArrayViewFactory = factory;
}

export function wrapFloat32ArrayView(view: Float32Array): Float32Array {
  if (float32ArrayViewFactory == null) {
    return view;
  }

  return float32ArrayViewFactory(view.buffer, view.byteOffset, view.length);
}
