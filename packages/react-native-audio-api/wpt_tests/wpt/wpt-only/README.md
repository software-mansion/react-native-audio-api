# WPT-only shims

Code in this directory exists solely for the Node WPT harness. It must not be
imported from `src/` or shipped in the library package.

Typical contents: runtime behaviors that TypeScript already enforces for
library consumers (e.g. `readonly` channel attributes) but that plain-JS WPT
pages still require via `assert_throws_dom` / audit.js.
