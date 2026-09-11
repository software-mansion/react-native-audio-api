import { normalizeTags, tagClassName } from '../../sidebarTags';

export function applySidebarTagClassNames(items) {
  if (!Array.isArray(items)) {
    return items;
  }

  return items.map((item) => {
    const tags = normalizeTags(item.customProps?.tags);
    const className = [item.className, ...tags.map(tagClassName)]
      .filter(Boolean)
      .join(' ');
    const nestedItems = applySidebarTagClassNames(item.items);

    if (className === item.className && nestedItems === item.items) {
      return item;
    }

    return {
      ...item,
      className,
      ...(nestedItems !== item.items ? { items: nestedItems } : {}),
    };
  });
}
