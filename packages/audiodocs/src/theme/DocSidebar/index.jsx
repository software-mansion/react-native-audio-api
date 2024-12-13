import React from 'react';
import { DocSidebar } from '@swmansion/t-rex-ui';

export default function DocSidebarWrapper(props) {
  const titleImages = {};

  const heroImages = {};

  const newItems = ['animations/withClamp'];
  const experimentalItems = ['shared-element-transitions/overview'];

  return (
    <DocSidebar
      newItems={newItems}
      experimentalItems={experimentalItems}
      heroImages={heroImages}
      titleImages={titleImages}
      {...props}
    />
  );
}
