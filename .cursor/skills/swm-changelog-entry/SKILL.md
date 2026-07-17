---
name: swm-changelog-entry
description: Create or update a changelog entry for a Software Mansion package/library in Strapi from release details a team provides — a version bump, a set of fixes, a PR list, pasted release notes, or "we shipped X, add a changelog". Use this whenever someone wants to add, write, draft, or update a changelog/release-notes entry for an SWM library (Reanimated, Worklets, ExecuTorch, Fishjam, Smelter, Scarb, etc.), even if they don't say "Strapi" or "changelog entry". Designed for external teams working with a scoped token limited to the changelog-entry type. Bakes in Software Mansion's tone of voice so the copy reads right. Trigger on "add a changelog for our 1.4 release", "write release notes for X", "log this bump in the changelog", "update the changelog with these fixes".
---

# Software Mansion changelog entry

This skill is for teams that maintain an SWM package and need to add a changelog entry from whatever release details they have — a version number, a date, a list of changes, a link to a GitHub release. It assumes you're working through the `strapi-mcp` connector with a **scoped token that can read and write `changelog-entry`** and probably nothing else. You may be able to `list`/`get` packages to find one to link to; you may not. Don't assume broader access — if a tool returns a permission error, work with what the token allows and tell the user what you couldn't do.

The goal is a clean, correctly-shaped, draft entry that reads like Software Mansion wrote it. Create as a draft by default and only publish if the user asks and the token permits it.

## The entry shape

`changelog-entry` fields:

- `title` (required) — the release name, e.g. `Reanimated 4.5.1`, `Fishjam 1.2.0`. Package name + version.
- `slug` (required) — kebab-case, e.g. `reanimated-4-5-1`. Replace dots with hyphens (`starknet.py` → `starknet-py`). Keep it unique and predictable.
- `releaseType` (required) — one of `major`, `minor`, `patch`, `announcement`. Map from semver: `1.0.0`→major, `1.4.0`→minor, `1.4.2`→patch. Use `announcement` for posts that aren't a version bump.
- `releaseDate` (required) — ISO date, e.g. `2026-07-02`. Use the actual release date, not today, if they differ.
- `version` — the bare version string, e.g. `4.5.1`.
- `summary` — one line shown on the entry card and the social/OG preview. Keep it to ~80–110 characters so it fits 1–2 lines. This is the highest-leverage field; see the voice section.
- `postContent` — the body (see below).
- `searchableContent` — a plain-text version of the notes for search indexing. Fill it with the change list as prose/plain text (no HTML).
- `packages` — relation to the package this belongs to. If you can resolve the package's `documentId`, link it with `{"packages": {"connect": ["<documentId>"]}}`. A bare id targets the draft version, which is right for a draft entry. **Always verify the link actually stuck** (see "Verify the package relation" below) — a scoped token without Package permissions drops the connect silently, with no error.
- `seo` — (required) Set it on the initial create: `metaTitle` (usually the entry title, keep it under ~60 characters) and `metaDescription` (adapt the `summary`, roughly 50–160 characters). Both fields are mandatory once the component is present, and the live site expects them on every published entry.

## The body: postContent blocks

`postContent` is a dynamic zone. The block you'll use almost every time is:

```json
{ "__component": "post.content", "content": "<h3>What's changed</h3><ul><li>...</li></ul>" }
```

`content` is **HTML rich text**. Allowed tags: `h3`, `h4`, `p`, `ul`/`ol`/`li`, `strong`, `em`, `code`, `pre`, `blockquote`, `a` (with `href`), `br`. Downgrade any `h1`/`h2` from pasted notes to `h3` so they sit under the title. Group changes under short section headings (Android / iOS / All platforms, or Features / Fixes) when there are enough of them to warrant it; a short patch can be a single bulleted list.

Two other blocks exist if you need them: `post.quote` (`content` required, plus `authorName`, `authorPosition`) for a maintainer pull-quote, and `post.callout` (`title`, `content`, and a required `block` sub-component) for a highlighted note — callouts are fiddly, so only reach for one when a normal paragraph won't do, and check its component schema first.

Linkify PR/issue references to the repo (`<a href="https://github.com/software-mansion/<repo>/pull/9746">#9746</a>`) when you have the repo. Write contributor mentions as plain text (`@name`), not as links to hovercards.

`postContent` writes go straight through the MCP `create`/`update` — pass the array of `{__component, ...}` objects. A validation error here is almost always a wrong `__component` name or a field that doesn't exist on that component, not a permissions issue.

## Verify the package relation (and what a silent failure means)

The package relation is load-bearing: the live site generates each changelog page from it, so an entry without a linked package either never gets built or isn't routable, and the published URL 404s even though Strapi reports the entry as published.

The failure mode to watch for is **silent**: with a token that lacks permissions on the Package content type, `packages.connect` doesn't error — the create/update succeeds and the relation is simply dropped. So after any create or update where you connected a package, `get` the entry back and check that `packages` is actually populated.

If it comes back empty:

1. Try once more with the package's `documentId` (not the numeric id) in case that was the issue.
2. If it's still empty, stop retrying — id-format changes won't fix a permissions gap. Tell the user plainly: **the token is missing permissions on the Package content type**, so relation writes are being silently ignored, and they should contact whoever issued the token and ask for Package read access plus permission to write the relation. Keep the entry as a draft in the meantime.

If you can't even `list`/`get` packages to resolve one, ask the user for the package slug or documentId — but the verification step above still applies, because being able to name a package doesn't mean the token can link it.

## If details are thin, ask for the essentials

You can't write a good entry from "add a changelog". Before creating, make sure you have: the **package/library name**, the **version**, the **release date**, and the **notable changes** (a list, pasted notes, or a GitHub release link). If they hand you a GitHub release URL or tag, read the notes from it and build the body from that. If something's missing and you can't infer it, ask — one round of specific questions beats guessing.

## Write it in Software Mansion's voice

The audience is other developers. They can smell press-release copy instantly and scroll past it. Every line has to give a concrete reason to care. These rules apply mostly to the `summary` and to any prose you write in the body (section intros, an announcement).

**Lead with the concrete thing.** The best summaries name what actually changed and what the reader gets. "CSS Core Animation gains shadow, background & border props, plus pseudo-selectors like :hover" beats "This release includes various improvements and enhancements."

**Cut these on sight:**
- "Excited to announce / thrilled to share / happy to introduce" — open with the news itself.
- "Cutting-edge, groundbreaking, world-class, next-generation" — empty intensifiers. If it's good, show why.
- "In today's fast-moving world / more important than ever" — generic scene-setting that adds nothing.
- A bare feature list with no context, or a bare benchmark number with no anchor — tell the reader why it matters ("transcribe a 10-min meeting in 8s instead of 30s", not "2× faster").

**Voice:**
- Write full, complete sentences. Never the clipped staccato style ("Faster builds. Fewer crashes. Zero config.") — a fragment gestures at a benefit without ever stating one, and it reads as ad copy. Say the actual thing in a real sentence.
- No em-dashes. Use a comma, a period, or parentheses.
- No "not just X, but Y" / "it's not X, it's Y" parallelisms — just say Y.
- Prefer positive framing: "runs fully offline" over "no internet required"; "everything stays on device" over "nothing leaves your machine". Reach for the upside first; keep a negation only when the absence genuinely is the punchline.
- Confident, not humble-bragging and not hype. "We're proud of this one, it fixes an ANR many teams hit on the New Architecture" beats both "just a small update" and "revolutionary breakthrough".
- Casual register is fine when it fits (a lowercase opening, a parenthetical aside, a small joke), but a changelog summary usually wants to be plain and precise.

**Summary examples:**
- Good (minor): `Bundle Mode is now stable; new importForwarding API replaces workletizableModules.`
- Good (patch): `Android fixes: RN 0.86 freezes, a CSS/Fabric ANR deadlock, and strokeDasharray crashes.`
- Weak: `We are excited to share various bug fixes and performance improvements in this release.` (generic, "excited to share", says nothing concrete)

If the full `software-mansion-tone-of-voice` skill is available, defer to it for anything longer than a summary (an announcement entry, a blog-style writeup).

## Workflow

1. Gather the essentials (name, version, date, changes). Read a GitHub release if they linked one.
2. Decide `releaseType` from the version, build `slug` and `title`.
3. Write a one-line `summary` in the voice above, and derive `seo.metaTitle` / `seo.metaDescription` from the title and summary.
4. Build `postContent` HTML from the changes (sectioned list, linkified PRs), and fill `searchableContent` with the plain-text version.
5. Resolve the package and include the `packages` connect in the create.
6. `create_changelog-entry` — it lands as a draft. Read it back with `get` and confirm three things: `postContent` looks right, `seo` is filled, and `packages` actually linked. If the package didn't link, follow "Verify the package relation" above before doing anything else.
7. Report what you made, keep it a draft, and publish only if asked and allowed — and only once the pre-publish checks in the guardrails pass.

## Guardrails

- **Draft by default.** Publishing pushes to the live site; do it only on explicit request, and only if the token has publish permission (a permission error means it doesn't — say so).
- **Never publish without the package linked and SEO filled.** The site builds changelog pages from the package relation, so publishing an unlinked entry produces a live 404 that looks like a site bug (this has happened; re-publishing after the link was added fixed it). If the user asks to publish and `packages` is empty or `seo` is missing, hold off, explain why, and fix the entry (or get the token fixed) first.
- **Don't invent changes.** Only include what the user provided or what's in the linked release. If you're unsure whether something shipped, leave it out and ask.
- **Stay in your lane.** This token is for changelog entries. Don't attempt to modify packages, other content types, or settings; if the task needs that, tell the user it's outside this scope.
- **One package per entry, usually.** If a monorepo ships two packages (e.g. Reanimated and Worklets from the same repo), that's two entries linked to two packages, not one entry covering both.
