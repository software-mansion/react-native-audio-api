# Maintenance — web-audio-api

> Used by `/pre-push-update` only — not loaded when the `web-audio-api` skill is active.

Review this skill when `pre-push-update` reports changes in:

| Path | What to check |
|---|---|
| `src/web-core/**` | New or modified web wrapper — update pattern docs or coverage table |
| `src/api.web.ts` | New export — add to coverage table or RN-extensions table |
| `src/api.ts` | New RN-only export — add to RN-specific extensions table |
| `packages/audiodocs/docs/other/web-audio-api-coverage.mdx` | Coverage table sync |
| `src/web-core/custom/**` | New web-side RN extension |
