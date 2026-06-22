import React from 'react';
// eslint-disable-next-line import/no-unresolved
import useBaseUrl from '@docusaurus/useBaseUrl';
// eslint-disable-next-line import/no-unresolved
import { useLocation } from '@docusaurus/router';
import { Navbar, TopbarBanner, isBannerHidden } from '@swmansion/t-rex-ui';
import { TOP_BAR_BANNER } from '@site/src/components/topbarBanner.config';
import './styles.css';

export default function NavbarWrapper(props) {
  const location = useLocation();
  const bannerHidden = isBannerHidden(
    location.pathname,
    TOP_BAR_BANNER.hiddenPaths
  );

  const titleImages = {
    light: useBaseUrl('/img/title.svg?v=12'),
    dark: useBaseUrl('/img/title-dark.svg?v=12'),
  };

  const heroImages = {
    logo: useBaseUrl('/img/logo-hero.svg'),
  };

  return (
    <div style={{ display: 'flex', flexDirection: 'column', flexShrink: 0 }}>
      {!bannerHidden && (
        <TopbarBanner
          zones={TOP_BAR_BANNER.zones}
          rotateIntervalMs={TOP_BAR_BANNER.rotateIntervalMs}
        />
      )}
      <Navbar
        useLandingLogoDualVariant={true}
        heroImages={heroImages}
        titleImages={titleImages}
        landingItems={[
          {
            href: '/react-native-audio-api/docs',
            label: 'Docs',
            position: 'right',
            'aria-label': 'Documentation',
          },
        ]}
        {...props}
      >
        <button type='button' className='navbar__toggle' aria-label='Toggle navigation'>
          <span className='navbar__toggle-icon' />
        </button>
      </Navbar>
    </div>
  );
}
