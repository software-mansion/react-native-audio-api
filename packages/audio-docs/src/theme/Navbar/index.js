import React from 'react';
import useBaseUrl from '@docusaurus/useBaseUrl';
import { Navbar } from '@swmansion/t-rex-ui';

export default function NavbarWrapper(props) {
  const titleImages = {
    light: useBaseUrl('/img/title.svg'),
    dark: useBaseUrl('/img/title-dark.svg'),
  };

  const heroImages = {
    logo: useBaseUrl('/img/logo-hero.svg'),
    title: useBaseUrl('/img/title-hero.svg'),
  };

  return (
    <Navbar
      {...props}
      isAlgoliaActive={false}
      heroImages={heroImages}
      titleImages={titleImages}
      useLandingLogoDualVariant
    />
  );
}
