import { JSX, useEffect } from 'react';
import {HireUsSection} from '@swmansion/t-rex-ui'
import './styles.module.css';

const HireUsSectionWrapper = ({
  content,
  href,
}: {
  content?: string | JSX.Element;
  href: string;
}) => {
   const resolvedContent = content || (
    <>
      We’re a software company built around improving developer experience and
      bringing innovative clients' ideas to life. We're pushing boundaries and
      delivering high-performance solutions that scale.
      <br />
      <br />
      Need help integrating React Native Audio API into your project or want to
      discuss your ideas?
    </>
  );

   // TODO: Remove this hack after we add support for custom button text in the HireUsSection component
   // Optional TODO: Contribute to t-rex-ui to add support for custom button text and remove this hack
   useEffect(() => {
    const btn = document.querySelector("[class*='hireUsSection'] [class*='homepageButtonLink'] > [class*='homepageButton']");
    if (btn) {
      const textNode = Array.from(btn.childNodes).find(n => n.nodeType === Node.TEXT_NODE);
      if (textNode) {
        textNode.textContent = "Let's talk";
      }
    }
  }, []);

    return <HireUsSection content={resolvedContent} href={href} />
};

export default HireUsSectionWrapper;
