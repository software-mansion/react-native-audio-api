import { TAG_LABEL } from '../sidebarTags';

const TAG_CLASS_PREFIX = 'internaldocs-tag-';

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
      return;
    }

    if (!list) {
      list = document.createElement('span');
      list.className = 'internaldocs-tag-list';
      link.appendChild(list);
    }

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

function scheduleInject() {
  requestAnimationFrame(injectSidebarTags);
}

function watchSidebar() {
  scheduleInject();
  const menu = document.querySelector('.theme-doc-sidebar-menu, .menu__list');
  if (!menu || menu.dataset.internaldocsTagWatch === 'true') {
    return;
  }
  menu.dataset.internaldocsTagWatch = 'true';
  new MutationObserver(scheduleInject).observe(menu, {
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
