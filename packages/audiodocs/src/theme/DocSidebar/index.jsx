import React from 'react';
import useBaseUrl from '@docusaurus/useBaseUrl';
import { DocSidebar } from '@swmansion/t-rex-ui';

export default function DocSidebarWrapper(props) {
  const titleImages = {
    light: useBaseUrl('/img/title.svg?v=12'),
    dark: useBaseUrl('/img/title-dark.svg?v=12'),
  };

  const heroImages = {
    logo: useBaseUrl('/img/logo-hero.svg'),
  };

  const newItems = [];
  const experimentalItems = [];
  const unreleasedItems = [];

  return (
    <DocSidebar
      newItems={newItems}
      experimentalItems={experimentalItems}
      unreleasedItems={unreleasedItems}
      heroImages={heroImages}
      titleImages={titleImages}
      {...props}
    />
  );
}
