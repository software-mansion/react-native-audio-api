---
name: code-comments
description: >
  Guidelines for writing comments in C++, TypeScript, Kotlin, and Objective-C++.
  Prefer self-explanatory code and names; comment non-obvious WHY — races, invariants,
  thread constraints, allocation boundaries, and Web Audio spec deviations. Apply when adding,
  editing, reviewing, or removing comments during any implementation or review task.
  Trigger phrases: "comment style", "add a comment", "write comments", "comment guidelines",
  "document this", "too many comments", "remove comment", "JSDoc", "Doxygen",
  "thread annotation".
---

# Skill: Code Comments

Prefer clear code over comments. Comment **why**, not **what** — races, invariants, thread and allocation boundaries, and intentional spec deviations. Skip comments that restate the next line of code.

---

## When to Comment (MUST)

Add a comment when any of the following would be non-obvious to a reader six months later:

| Situation | What to capture |
|---|---|
| Thread constraint | Which thread may call / mutate; if JS must use `scheduleAudioEvent` |
| Race / ordering invariant | Failure mode if violated (e.g. Channel A before Channel B) |
| Allocation / realtime boundary | What is forbidden on the audio thread and where work is prebuilt |
| Spec deviation or conformance trick | Cite Web Audio / MDN; explain the intentional difference |
| Magic number / formula | Source of the constant and why the bound exists |
| Cross-language contract | JSI / native / TS alignment that types cannot express |

## When NOT to Comment (MUST NOT)

- Restate the identifier or the next statement (`// increment i`, `/** Starts recording */` with no gotchas).
- Leave commented-out code — delete it; git keeps history.
- Narrate every step of an obvious algorithm.
- Duplicate a type signature in prose.
- Add changelog / authorship comments (`// Fixed bug`, `// Added by …`).
- Write a TODO without an actionable next step (and preferably an issue or concrete constraint).

---

## Hard Rules

1. **WHY over WHAT** — if the comment only repeats the code, delete it.
2. **Prefer self-explanatory names** — rename before commenting around a bad name.
3. **Keep comments adjacent to the constraint** — put thread / race notes on the declaration that carries the risk, not in a distant file header.
4. **Update or delete on edit** — a wrong comment is worse than none.
5. **Normalize thread tags etc.** (see below) — do not invent new spellings.
6. **Language-appropriate markup** — `///` / `@note` in C++ headers; JSDoc in public TS; short `//` for local WHY.

---

## C++ Style

- Use `///` (Doxygen) on public / shared headers for non-obvious API: `@brief`, `@note`, `@param` when they add information the signature does not.
- Use `//` for local WHY inside `.cpp` (races, ordering, “do not reorder”).
- File- or class-level `///` blocks are appropriate for coordinators with multi-thread contracts (`Graph.h`).
- Prefer one dense paragraph that states the invariant and the failure mode over a wall of restated members.

```cpp
/// @brief Sets `channelInterpretation`. Read on the audio thread in
/// `processInputs`, so it MUST be applied via a scheduled audio event —
/// do not mutate directly from the JS thread.
/// @note Audio-thread only.
```

---

## TypeScript Style

- Public / exported APIs: JSDoc only when there are gotchas, units, ranges, or identity/lifetime rules the types do not capture.
- Internal bridges: `/** @internal */` is fine; still add a WHY comment for native/spec quirks.
- Prefer a short `//` above the implementation detail over empty JSDoc that restates the method name.

```typescript
// Per the Web Audio spec, getChannelData() must return the same Float32Array
// for a given channel across calls. The native getter creates a fresh view
// each time, so cache it here.
private readonly channelDataCache: (Float32Array<ArrayBuffer> | undefined)[] = [];
```

---

## Kotlin / Objective-C++

- Same WHY rules as C++.
- Annotate JNI / ObjC++ entry points that must run on a specific thread or that hop to the audio graph.
- Do not duplicate C++ header docs on thin wrappers — point to the C++ contract or document only the platform-specific delta.

---

## TODOs

Allowed when they are **actionable**:

```cpp
// TODO: free the previous buffer on the audio thread after the swap settles
// (see AudioBufferHostObject memory-pressure notes).
```

Reject:

```cpp
// TODO: fix this later
// TODO: maybe optimize
```

---

## Anti-Patterns (examples)

```cpp
// BAD — narrates the code
i++; // increment i

// BAD — commented-out code
// auto old = gain_;
// gain_ = value;

// BAD — weak TODO
// TODO: handle edge case

// GOOD — invariant + failure mode
// Drain Channel A before Channel B so addNode(X) happens-before orphan(X).
```

```typescript
// BAD
/** Starts the audio recording. */
start(): void;

// GOOD — documents a non-obvious constraint
/**
 * Starts recording. `bufferLength` below ~256 frames increases callback
 * frequency and can stall the JS thread on busy devices.
 */
start(): void;
```

---

## Checklist Before Leaving Comments in a Diff

- [ ] Every new comment explains WHY or a constraint types cannot express
- [ ] Spec / race / allocation notes cite the failure mode
- [ ] No commented-out code, changelog comments, or empty JSDoc
- [ ] Comments on edited code were updated or removed

*Maintenance: see [maintenance.md](maintenance.md).*
