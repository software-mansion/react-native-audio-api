# Internal docs

Local Docusaurus site for engine internals. It is not hosted. Public API docs are in `packages/audiodocs`.

It is a workspace package: run `pnpm install` from the monorepo root.

## Run

From this directory:

```bash
pnpm start
```

From the monorepo root:

```bash
pnpm docs:internal
```

If `audiodocs` is already on port 3000, use `pnpm start --port 3001`.

Production build (used by CI):

```bash
pnpm build
```
