---
name: flow
description: >
  End-to-end process for shipping a feature or bug fix in react-native-audio-api. Covers all required deliverables in order: Web Audio API spec review, TypeScript interface and types, C++ AudioNode implementation, HostObject wiring, TypeScript class, TurboModule spec (when needed), C++ tests (Google Test), JS tests (Jest), documentation (audiodocs MDX), and post-work checks. Also covers the bug-fix flow: MRE first, C++ test when applicable, root-cause analysis, post-mortem. Use this skill at the start of any feature implementation or bug fix. Trigger phrases: "implement a feature", "add a node", "fix a bug", "what steps", "where do I start", "PR checklist", "how to write tests".
---

# Skill: Feature Implementation Flow

## Quick Reference

**Feature checklist (all 9 steps required for a PR):**
1. Read spec → define exactly what you're building
2. TypeScript interface + option types (`src/interfaces/`, `src/types/`)
3. C++ AudioNode (`core/<category>/MyNode.h/.cpp`) — see `audio-nodes` skill
4. HostObject (`HostObjects/MyNodeHostObject.h/.cpp`) — see `host-objects` skill
5. TypeScript class (`src/core/MyNode.ts`)
6. TurboModule spec entry — **only** if a new RN native method is needed
7. C++ tests (Google Test) — **write tests first, confirm they fail, then implement**
8. JS/TS tests (Jest)
9. Docs page (`packages/audiodocs/`)
Then: `post-work-checks` skill

**Bug fix checklist:**
1. Reproduce (write failing test or MRE first)
2. Identify the layer (use the table in §Bug Fix Flow)
3. Fix + verify tests pass
4. Post-mortem (update skill if a new pitfall discovered)

---

## Feature Implementation Flow

### 1 — Define exactly what you are building

Before touching any code. **If spec behavior is unclear → fetch https://webaudio.github.io/web-audio-api/ before proceeding.**

- Read the Web Audio API spec section for the node/feature. Nail down:
  - All properties and methods
  - Parameter default values, min/max, and automation behaviour
  - Edge cases (what happens when `start()` is called twice? what if feedback[0] === 0?)
- Decide the **TypeScript interface shape**: what does the JS consumer see?
- Identify if **native platform code** is needed (e.g. new permission, device routing, new Oboe/CoreAudio API surface). If yes, plan iOS + Android separately.

---

### 2 — TypeScript interface and types

Files: `packages/react-native-audio-api/src/`

1. Add the interface in `src/interfaces/` — `IMyNode.ts`
   ```ts
   export interface IMyNode extends IAudioNode {
     readonly myParam: IAudioParam;
     someMethod(arg: number): void;
   }
   ```

2. Add the options type in `src/types/` — `NodeOptions.ts` (or add to existing file)
   ```ts
   export interface MyNodeOptions extends AudioNodeOptions {
     myParam?: number; // default: 1.0
   }
   ```

3. Export both from `src/interfaces/index.ts` and `src/types/index.ts`.

---

### 3 — C++ AudioNode

Files: `packages/react-native-audio-api/common/cpp/audioapi/core/`

See the `audio-nodes` skill for the full contract. **If unsure which base class to use → check the class hierarchy in that skill.** Summary:

1. Create `core/<category>/MyNode.h` and `MyNode.cpp`.
2. Subclass the right base (`AudioNode`, `AudioScheduledSourceNode`, `AudioBufferBaseSourceNode`).
3. Annotate every method with `// JS-thread only` or `// Audio-thread only`.
4. Declare `processNode()` in `protected:` — audio thread.
5. Preallocate all `AudioParam`s and scratch buffers in the constructor (JS thread).
6. Add `createMyNode(const MyNodeOptions &options)` factory to `BaseAudioContext`.
7. Add the `.cpp` file to `CMakeLists.txt` (check existing entries for the pattern).

**No allocations, no locks, no blocking I/O in `processNode()`.**

---

### 4 — HostObject

Files: `packages/react-native-audio-api/common/cpp/audioapi/HostObjects/`

See the `host-objects` skill for the full macro system and shadow state patterns. **If unsure whether to use shadow state or atomics → check the decision table in that skill.** Summary:

1. Create `HostObjects/MyNodeHostObject.h` and `MyNodeHostObject.cpp`.
2. Extend `AudioNodeHostObject`.
3. Expose properties with `JSI_PROPERTY_GETTER/SETTER_DECL/IMPL` + `JSI_EXPORT_PROPERTY_GETTER/SETTER`.
4. Expose methods with `JSI_HOST_FUNCTION_DECL/IMPL` + `JSI_EXPORT_FUNCTION`.
5. Use **shadow state** (JS-thread copy + `scheduleAudioEvent`) for any property that: is read back by JS, is written from JS, AND must be applied on the audio thread. See `host-objects.md` for the decision table.
6. Wire factory in `BaseAudioContextHostObject`: add `createMyNode` host function that calls `context->createMyNode(options)` and wraps in `MyNodeHostObject`.
7. Add to `CMakeLists.txt`.

---

### 5 — TypeScript class

Files: `packages/react-native-audio-api/src/core/`

1. Create `src/core/MyNode.ts` — extends `AudioNode`, wraps the JSI HostObject.
   ```ts
   import { IMyNode } from '../interfaces';
   import AudioNode from './AudioNode';
   import BaseAudioContext from './BaseAudioContext';
   import { MyNodeOptions } from '../types';

   export default class MyNode extends AudioNode implements IMyNode {
     constructor(context: BaseAudioContext, options: MyNodeOptions) {
       const node = context.context.createMyNode(options);
       super(context, node);
     }
     // getters/setters forwarded to (this.node as IMyNode)
   }
   ```

2. Add a factory method `createMyNode(options?)` to `src/core/BaseAudioContext.ts`.

3. Export from `src/index.ts`.

4. Add web passthrough in `src/web-core/MyNode.ts` if the node has a browser equivalent, or add a stub/throw if not supported on web. The web system always delegates to the browser's `AudioContext`.

---

### 6 — TurboModule spec (only if new native entry point needed)

File: `src/specs/NativeAudioAPIModule.ts`

Only needed if the feature requires a *new* RN native method (e.g. a permission check, a new audio session configuration). Most audio nodes do **not** need this — they are exposed entirely through JSI HostObjects.

If needed: add the method signature to the spec, then implement on iOS (`AudioAPIModule.mm`) and Android (`AudioAPIModule.kt`).

---

### 7 — Tests

#### C++ tests (Google Test)

Path: `common/cpp/test/src/core/<category>/MyNodeTest.cpp`

Pattern:
```cpp
#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/effects/MyNode.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <memory>
using namespace audioapi;

class MyNodeTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;
  static constexpr int sampleRate = 44100;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(
        2, 5 * sampleRate, sampleRate, eventRegistry, RuntimeRegistry{});
    context->initialize();
  }
};

// Use TestableXxx subclass to expose processNode() and internal state for white-box tests
class TestableMyNode : public MyNode {
 public:
  explicit TestableMyNode(std::shared_ptr<BaseAudioContext> ctx)
      : MyNode(ctx, MyNodeOptions()) {}
  std::shared_ptr<AudioBuffer> processNode(
      const std::shared_ptr<AudioBuffer> &buf, int frames) override {
    return MyNode::processNode(buf, frames);
  }
};

TEST_F(MyNodeTest, CanBeCreated) { ... }
TEST_F(MyNodeTest, ProcessesAudioCorrectly) { ... }
TEST_F(MyNodeTest, EdgeCase) { ... }
```

New test files are picked up automatically by `GLOB_RECURSE` — no CMakeLists edit needed.

Run with:
```bash
yarn test   # from repo root
```

#### JS/TS tests (Jest)

Path: `packages/react-native-audio-api/tests/`

- `integration.test.ts` — graph construction, node creation, property access through the mock API
- `mock.test.ts` — mock implementation correctness

Run with:
```bash
yarn workspace react-native-audio-api test
```

#### Example app (fabric-example)

If the feature has a visible or audible component, add a demo in `apps/fabric-example/App.tsx` (or a sub-screen). This acts as a manual smoke test and documents intended usage.

---

### 8 — Documentation

Path: `packages/audiodocs/`

Every **public API** that ships must have a documentation page. The docs package uses MDX. Look at an existing node's page for the format. Cover:

- Constructor / factory call
- All properties and their types/ranges
- All methods with parameter descriptions
- A minimal usage example (TypeScript snippet)
- Any spec deviations or limitations (e.g. "offline rendering not supported")

---

### 9 — Post-work checks

See the `post-work-checks` skill. At minimum before any PR:

```bash
yarn format          # auto-fix formatting
yarn lint            # JS/TS/C++/Kotlin linting
yarn typecheck       # TypeScript
yarn test            # C++ Google Tests
yarn check-audio-enum-sync  # if you touched AudioEvent enums
```

---

## Bug Fix Flow

### 1 — Reproduce (MRE first)

- **If an MRE is provided** → good, use it as your starting point.
- **If no MRE** → reproduce in `apps/fabric-example/App.tsx`. Add the minimal code that triggers the bug. Keep it in the app until the fix is verified, then remove or clean it up.
- **If the bug is in C++ DSP/processing logic** → write a **failing C++ test first** (`common/cpp/test/`). This gives a fast edit-compile-run loop without needing a device, and the test stays as a permanent regression guard.

---

### 2 — Identify the layer

| Symptom | Likely layer | Where to look |
|---|---|---|
| Wrong default value / validation error | TypeScript | `src/core/MyNode.ts`, `src/types/` |
| Property read-back returns wrong value | HostObject shadow state | `HostObjects/MyNodeHostObject` |
| Wrong argument parsed from JS | HostObject | argument parsing in `get`/`call` |
| Audio output sounds wrong | C++ `processNode()` | `core/<category>/MyNode.cpp` |
| DSP math wrong | DSP helpers | `dsp/` utilities |
| Crash on audio thread | Thread safety violation | `cross-thread` patterns — `audio-nodes.md` |
| Platform-specific (only iOS or only Android) | Native layer | `ios/` or `android/` |
| Only in new RN architecture | TurboModule / JSI wiring | `src/specs/`, HostObject init |

---

### 3 — Fix and verify

1. Make the fix.
2. If a C++ test was written in step 1 → it must now pass.
3. If no C++ test was written but the fix touches `processNode()` or DSP code → add one now. It will stay as a regression test.
4. Run post-work checks (see above).

---

### 4 — Post-mortem (short, but do it)

After the fix is working, spend 2 minutes asking:

- **Why did this bug exist?** Was it a missing validation? A wrong default value? A thread-safety assumption that wasn't documented?
- **Should a skill file be updated?** If the bug revealed a non-obvious invariant (e.g. "feedback[0] must not be 0"), add it to the relevant skill (`audio-nodes.md`, `host-objects.md`, etc.) under a pitfalls or constraints section.
- **Should a new test be added?** If no test existed that would have caught this, add one.
- **Should docs be updated?** If the spec says X but the implementation did Y, and the fix brings it in line — update the docs page.

These small post-mortems compound over time and prevent the same class of bug from recurring.

---

*Maintenance: see [maintenance.md](maintenance.md).*
