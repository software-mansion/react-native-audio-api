# C++ tests

`common/cpp/test/` — Google Test, modes by **runtime cost**.

| Mode | What | Default binary |
| --- | --- | --- |
| **smoke** | Fast suites (PR + coverage). Disjoint from extended. | `tests` |
| **extended** | Slow suites by **category** | `tests_asan` (ASan+UBSan) |
| **full** | smoke, then all extended categories | each mode’s default |

```bash
yarn test:cpp                          # smoke
yarn test:cpp:extended                 # all categories
yarn test:cpp:extended -- graph
yarn test:cpp:full
yarn test:cpp:coverage                 # smoke + llvm-cov HTML
yarn test:cpp:smoke --ubasan           # Address + UndefinedBehavior sanitizers
yarn test:cpp:extended -- graph --tsan
yarn test:cpp:extended -- graph --no-ubasan
bash common/cpp/test/RunTests.sh --help
```

`--ubasan` = AddressSanitizer + UndefinedBehaviorSanitizer. Incompatible with `--tsan` (ASan and TSan cannot run together).

Filters: [`filters.sh`](filters.sh). Override: `GTEST_FILTER=...`.

### Categories

| Category | Contents | CI |
| --- | --- | --- |
| `graph` | Slow graph (`AudioGraph*`, `Graph*`, `HostGraph*`, `Seeds/*`, …) | path filter + manual dispatch in `tests.yml` |

**`GraphNodeGrowthTest` is smoke**, not `graph`: it is short (~100–200 ms) and needs unsanitized `AudioThreadGuard` (asserts `GTEST_SKIP` under ASan/TSan).

**Add a category:** (1) filter in `filters.sh`, (2) append name to `CPP_TEST_EXTENDED_CATEGORIES`, (3) in `tests.yml` add a `workflow_dispatch` boolean and one `cpp-extended-*` job that calls `cpp-extended-job.yml` with `categories`, `force`, and that category’s `path_filters`.

### Legacy aliases

`yarn test:graph` → `extended graph`. Docker: `yarn test:graph:docker` forwards args to `RunTests.sh` (default `extended graph`). `yarn validate:graph` → extended category `graph` only.
