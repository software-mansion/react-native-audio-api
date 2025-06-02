import React from 'react';
import useBaseUrl from '@docusaurus/useBaseUrl';
import { Navbar } from '@swmansion/t-rex-ui';

export default function NavbarWrapper(props) {
  const titleImages = {
    light: useBaseUrl('/img/title.svg?v=12'),
    dark: useBaseUrl('/img/title-dark.svg?v=12'),
  };

  const heroImages = {
    logo: useBaseUrl('/img/logo-hero.svg'),
  };

  return (
    <Navbar
      useLandingLogoDualVariant={true}
      heroImages={heroImages}
      titleImages={titleImages}
      {...props}
    />
  );
}
