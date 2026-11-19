# C++ tests

`common/cpp/test/` — Google Test, modes by **runtime cost**.

| Mode         | What                                                                         | Default binary            |
| ------------ | ---------------------------------------------------------------------------- | ------------------------- |
| **smoke**    | Fast suites (PR + coverage). Disjoint from extended.                         | `tests`                   |
| **extended** | Slow suites by **category**                                                  | `tests_asan` (ASan+UBSan) |
| **full**     | smoke, then all extended categories                                          | each mode’s default       |
| **affected** | the suites whose paths (in `filters.sh`) your changes touch; nothing if none | each mode’s default       |

```bash
pnpm test:cpp                          # smoke
pnpm test:cpp:extended                 # all categories
pnpm test:cpp:extended -- graph
pnpm test:cpp:full
pnpm test:cpp:affected                 # what `pnpm test` and CI run
pnpm test:cpp:coverage                 # smoke + llvm-cov HTML, lcov, totals
pnpm test:cpp:smoke --ubasan           # Address + UndefinedBehavior sanitizers
pnpm test:cpp:extended -- graph --tsan
pnpm test:cpp:extended -- graph --no-ubasan
bash common/cpp/test/run-tests.sh --help
```

`--ubasan` = AddressSanitizer + UndefinedBehaviorSanitizer. Incompatible with `--tsan` (ASan and TSan cannot run together).

Filters: [`filters.sh`](filters.sh). Override: `GTEST_FILTER=...`.

`affected` diffs against the base from `scripts/base-ref.sh`, the one `pnpm check` and CI use: `BASE_REF` if set, `HEAD^1` on a pull request in GitHub Actions (the checkout is a merge commit whose first parent is the base branch), and otherwise the merge-base with `origin/main`, counting uncommitted and untracked files. `TEST_ALL=1` runs every suite; CI sets it outside pull requests and when a shared path changes. If the base can't be resolved, every suite runs.

### Categories

| Category | Contents                                                         | CI                                          |
| -------- | ---------------------------------------------------------------- | ------------------------------------------- |
| `graph`  | Slow graph (`AudioGraph*`, `Graph*`, `HostGraph*`, `Seeds/*`, …) | `affected`, when graph code or tests change |

**`GraphNodeGrowthTest` is smoke**, not `graph`: it is short (~100–200 ms) and needs unsanitized `AudioThreadGuard` (asserts `GTEST_SKIP` under ASan/TSan).

**Add a category:** in `filters.sh`, (1) add its gtest filter to `cpp_test_extended_filter_for_category` and its paths to `cpp_test_paths_for_suite`, (2) append the name to `CPP_TEST_EXTENDED_CATEGORIES`. `affected` then runs it in CI when those paths change; no workflow edit is needed.

### Docker (Linux leak / ASan)

`pnpm test:cpp:docker` runs `run-tests.sh` in Docker (default `extended`). Extra args are forwarded. Use this from macOS when you need Linux ASan leak detection.
