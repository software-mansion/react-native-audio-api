import clsx from 'clsx';
import React, { useState } from 'react';

import styles from './styles.module.css';

interface ImageProps {
  alt: string;
  src: string;
}

interface Props {
  author: string;
  image?: ImageProps;
}

const initialsOf = (author: string): string => {
  const nameParts = author.trim().split(/\s+/).filter(Boolean);

  if (nameParts.length === 0) {
    return '';
  }

  const firstInitial = nameParts[0][0];
  const lastInitial =
    nameParts.length > 1 ? nameParts[nameParts.length - 1][0] : '';

  return (firstInitial + lastInitial).toUpperCase();
};

const AuthorAvatar = ({ author, image }: Props) => {
  const [hasImageFailedToLoad, setHasImageFailedToLoad] = useState(false);

  if (!image || hasImageFailedToLoad) {
    return (
      <div
        className={clsx(styles.authorAvatar, styles.authorInitials)}
        role="img"
        aria-label={author}
      >
        <span aria-hidden="true">{initialsOf(author)}</span>
      </div>
    );
  }

  return (
    <div className={styles.authorAvatar}>
      <img
        alt={image.alt}
        src={image.src}
        onError={() => setHasImageFailedToLoad(true)}
      />
    </div>
  );
};

export default AuthorAvatar;
