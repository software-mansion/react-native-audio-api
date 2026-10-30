import React from 'react';
import { normalizeTags, TAG_LABEL } from '../../sidebarTags';

type Props = {
  tags?: string | string[];
};

export default function SidebarTagBadge({ tags }: Props) {
  const list = normalizeTags(tags);
  if (list.length === 0) {
    return null;
  }

  return (
    <span className="internaldocs-tag-list">
      {list.map((tag) => (
        <span
          key={tag}
          className={`internaldocs-tag-badge internaldocs-tag-badge--${tag}`}
        >
          {TAG_LABEL[tag] ?? tag}
        </span>
      ))}
    </span>
  );
}
