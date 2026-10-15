import type { FC } from 'react';

type BadgeProps = {
  footnote?: boolean;
};

export const Optional: FC<BadgeProps>;
export const ReadOnly: FC<BadgeProps>;
export const Overridden: FC<BadgeProps>;
export const IOS: FC<BadgeProps>;
export const Android: FC<BadgeProps>;
export const Experimental: FC<BadgeProps>;
export const MobileOnly: FC<BadgeProps>;
