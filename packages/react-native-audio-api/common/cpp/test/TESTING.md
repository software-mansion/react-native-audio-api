# C++ tests

`common/cpp/test/` — Google Test, modes by **runtime cost**.

| Mode | What | Default binary |
| --- | --- | --- |
| **smoke** | Fast suites (PR + coverage). Disjoint from extended. | `tests` |
| **extended** | Slow suites by **category** | `tests_asan` (ASan+UBSan) |
| **full** | smoke, then all extended categories | each mode’s default |

```bash
pnpm test:cpp                          # smoke
pnpm test:cpp:extended                 # all categories
pnpm test:cpp:extended -- graph
pnpm test:cpp:full
pnpm test:cpp:coverage                 # smoke + llvm-cov HTML
pnpm test:cpp:smoke --ubasan           # Address + UndefinedBehavior sanitizers
pnpm test:cpp:extended -- graph --tsan
pnpm test:cpp:extended -- graph --no-ubasan
bash common/cpp/test/run-tests.sh --help
```

`--ubasan` = AddressSanitizer + UndefinedBehaviorSanitizer. Incompatible with `--tsan` (ASan and TSan cannot run together).

Filters: [`filters.sh`](filters.sh). Override: `GTEST_FILTER=...`.

### Categories

| Category | Contents | CI |
| --- | --- | --- |
| `graph` | Slow graph (`AudioGraph*`, `Graph*`, `HostGraph*`, `Seeds/*`, …) | path filter + manual dispatch in `tests.yml` |

**`GraphNodeGrowthTest` is smoke**, not `graph`: it is short (~100–200 ms) and needs unsanitized `AudioThreadGuard` (asserts `GTEST_SKIP` under ASan/TSan).

**Add a category:** (1) gtest filter and CI path globs in `filters.sh`, (2) append the name to `CPP_TEST_EXTENDED_CATEGORIES`, (3) in `tests.yml` add a `workflow_dispatch` boolean and one `cpp-extended-*` job that calls `cpp-job.yml` with `run: pnpm test:cpp:extended -- <category>`, `categories: <category>`, and `force`.

### Docker (Linux leak / ASan)

`pnpm test:cpp:docker` runs `run-tests.sh` in Docker (default `extended`). Extra args are forwarded. Use this from macOS when you need Linux ASan leak detection.
