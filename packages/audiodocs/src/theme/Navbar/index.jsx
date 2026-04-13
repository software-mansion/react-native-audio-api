import { useLocation } from '@docusaurus/router';
import React from 'react';
// eslint-disable-next-line import/no-unresolved
import useBaseUrl from '@docusaurus/useBaseUrl';
import TopPromoRotator, { PROMO_VERSION } from '@site/src/components/TopPromoRotator';
import { Navbar } from '@swmansion/t-rex-ui';
import './styles.css';

export default function NavbarWrapper(props) {
  const location = useLocation();
  const baseUrl = useBaseUrl('/');
  const isLanding = location.pathname === baseUrl;

  const [showPromo, setShowPromo] = React.useState(true);

  React.useEffect(() => {
    if (isLanding || typeof globalThis === 'undefined') {
      return;
    }

    try {
      const raw = globalThis.localStorage?.getItem('topPromoState');
      const state = raw ? JSON.parse(raw) : null;
      if (state?.v === PROMO_VERSION && state?.hidden) {
        setShowPromo(false);
      }
    } catch (_) {
      // ignore
    }
  }, [isLanding]);

  const handleClosePromo = React.useCallback(() => {
    setShowPromo(false);
    if (typeof globalThis !== 'undefined') {
      try {
        globalThis.localStorage?.setItem(
          'topPromoState',
          JSON.stringify({ v: PROMO_VERSION, hidden: true })
        );
      } catch {
        // ignore
      }
    }
  }, []);

  const titleImages = {
    light: useBaseUrl('/img/title.svg?v=12'),
    dark: useBaseUrl('/img/title-dark.svg?v=12'),
  };

  const heroImages = {
    logo: useBaseUrl('/img/logo-hero.svg'),
  };

  return (
    <div style={{ display: 'flex', flexDirection: 'column', flexShrink: 0 }}>
      {isLanding ? (
        <TopPromoRotator />
      ) : (
        showPromo && <TopPromoRotator onClose={handleClosePromo} />
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
