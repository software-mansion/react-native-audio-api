import { TAG_LABEL } from '../sidebarTags';

const TAG_CLASS_PREFIX = 'internaldocs-tag-';
const LABEL_CLASS = 'internaldocs-sidebar-label';

function wrapLinkLabel(link, list) {
  const strayNodes = [...link.childNodes].filter(
    (node) =>
      node !== list &&
      !(node instanceof Element && node.classList.contains(LABEL_CLASS))
  );
  let label = link.querySelector(`:scope > .${LABEL_CLASS}`);
  if (!label) {
    label = document.createElement('span');
    label.className = LABEL_CLASS;
    link.insertBefore(label, list);
  }
  strayNodes.forEach((node) => label.appendChild(node));
}

function unwrapLinkLabel(link) {
  const label = link.querySelector(`:scope > .${LABEL_CLASS}`);
  if (!label) {
    return;
  }
  label.replaceWith(...label.childNodes);
}

function injectSidebarTags() {
  document.querySelectorAll('.menu__list-item').forEach((item) => {
    const tags = [...item.classList]
      .filter((className) => className.startsWith(TAG_CLASS_PREFIX))
      .map((className) => className.slice(TAG_CLASS_PREFIX.length));
    const link = item.querySelector(':scope > .menu__link');
    if (!link) {
      return;
    }

    let list = link.querySelector(':scope > .internaldocs-tag-list');
    if (tags.length === 0) {
      list?.remove();
      unwrapLinkLabel(link);
      return;
    }

    if (!list) {
      list = document.createElement('span');
      list.className = 'internaldocs-tag-list';
      link.appendChild(list);
    }

    wrapLinkLabel(link, list);

    const signature = tags.join(' ');
    if (list.dataset.tags === signature) {
      return;
    }

    list.dataset.tags = signature;
    list.replaceChildren(
      ...tags.map((tag) => {
        const pill = document.createElement('span');
        pill.className = `internaldocs-tag-badge internaldocs-tag-badge--${tag}`;
        pill.textContent = TAG_LABEL[tag] ?? tag;
        return pill;
      })
    );
  });
}

let injectFrame = 0;

function scheduleInject() {
  if (injectFrame !== 0) {
    return;
  }
  injectFrame = requestAnimationFrame(() => {
    injectFrame = 0;
    injectSidebarTags();
  });
}

function watchSidebar() {
  scheduleInject();
  const root = document.documentElement;
  if (root.dataset.internaldocsTagWatch === 'true') {
    return;
  }
  root.dataset.internaldocsTagWatch = 'true';
  // Desktop and mobile sidebars are separate trees. The mobile menu is created
  // when the drawer opens, after the desktop menu (and any observer on it) is gone.
  new MutationObserver(scheduleInject).observe(root, {
    childList: true,
    subtree: true,
    attributes: true,
    attributeFilter: ['class'],
  });
}

export function onClientEntry() {
  watchSidebar();
}

export function onRouteDidUpdate() {
  watchSidebar();
}
