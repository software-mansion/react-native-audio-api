import React from 'react';
import NotFound from '@theme-original/NotFound';

type Props = Record<string, unknown>;

export default function NotFoundWrapper(props: Props) {
  return <NotFound {...props} />;
}
