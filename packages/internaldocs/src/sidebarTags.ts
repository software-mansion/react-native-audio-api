export const TAG_LABEL: Record<string, string> = {
  plan: 'Plan',
  wip: 'WIP',
  research: 'Research',
};

export function normalizeTags(value: unknown): string[] {
  const raw = Array.isArray(value)
    ? value
    : typeof value === 'string'
      ? value.split(/[\s,]+/)
      : [];

  return raw
    .map((tag) => String(tag).trim().toLowerCase())
    .filter(Boolean)
    .filter((tag, index, tags) => tags.indexOf(tag) === index);
}

export function tagClassName(tag: string): string {
  return `internaldocs-tag-${tag}`;
}
