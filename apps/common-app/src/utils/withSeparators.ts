import { ReactNode } from 'react';

type RenderElement<T> = (element: T, index: number) => ReactNode;
type RenderSeparator = (index: number) => ReactNode;

export default function withSeparators<T>(
  collection: ArrayLike<T>,
  renderSeparator: RenderSeparator,
  renderElement: RenderElement<T>
) {
  const elements = [];

  for (let i = 0; i < collection.length; i += 1) {
    elements.push(renderElement(collection[i], i));

    if (i < collection.length - 1) {
      elements.push(renderSeparator(i));
    }
  }

  return elements;
}
