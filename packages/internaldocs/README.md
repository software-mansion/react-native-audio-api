# Internal docs

Local Docusaurus site for engine internals. It is not hosted. Public API docs are in `packages/audiodocs`.

This package is a **standalone Yarn 4 project** (own `yarn.lock`), not a monorepo workspace. Install and run from this directory.

## Run

From this directory:

```bash
yarn install
yarn start
```

From the monorepo root:

```bash
yarn docs:internal
```

If `audiodocs` is already on port 3000, use `yarn start --port 3001`.

Production build (used by CI):

```bash
yarn build
```
