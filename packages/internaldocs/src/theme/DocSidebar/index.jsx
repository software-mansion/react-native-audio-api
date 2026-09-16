import useBaseUrl from '@docusaurus/useBaseUrl';
import { DocSidebar } from '@swmansion/t-rex-ui';
import React from 'react';
import { applySidebarTagClassNames } from './applySidebarTagClassNames';

export default function DocSidebarWrapper(props) {
  const titleImages = {
    light: useBaseUrl('/img/title.svg?v=12'),
    dark: useBaseUrl('/img/title-dark.svg?v=12'),
  };

  const heroImages = {
    logo: useBaseUrl('/img/logo-hero.svg'),
  };

  return (
    <DocSidebar
      newItems={[]}
      experimentalItems={[]}
      unreleasedItems={[]}
      heroImages={heroImages}
      titleImages={titleImages}
      {...props}
      sidebar={applySidebarTagClassNames(props.sidebar)}
    />
  );
}
