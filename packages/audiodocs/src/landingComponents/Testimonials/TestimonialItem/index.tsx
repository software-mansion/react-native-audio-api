import { useColorMode } from '@docusaurus/theme-common';
import React, { PropsWithChildren } from 'react';

import QuoteIcon from './QuoteIcon';
import styles from './styles.module.css';

interface ImageProps {
  alt: string;
  src: string;
}
interface Props extends PropsWithChildren {
  author: string;
  company?: string;
  link: string;
  image: ImageProps;
}

const TestimonialItem = ({ author, company, image, link, children }: Props) => {
  return (
    <div className={styles.testimonialItem}>
      <QuoteIcon
        className={styles.quoteIcon}
        color={
          useColorMode().colorMode === 'dark'
            ? 'var(--swm-red-dark-100)'
            : 'var(--swm-red-light-100)'
        }
      />
      <div className={styles.testimonialAuthor}>
        <div className={styles.testimonialAuthorPhoto}>
          <img alt={image.alt} src={image.src} />
        </div>
        <div className={styles.testimonialAuthorInfo}>
          <h5 className={styles.testimonialAuthorName}>{author}</h5>
          <a href={link} target="_blank" rel="noopener noreferrer" className={styles.testimonialAuthorLink}>
            <span className={styles.testimonialCompany}>{company}</span>
          </a>
        </div>
      </div>
      <p className={styles.testimonialBody}>{children}</p>
    </div>
  );
};

export default TestimonialItem;
