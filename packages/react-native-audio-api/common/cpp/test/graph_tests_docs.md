# Graph / extended tests (legacy note)

Canonical docs: [`TESTING.md`](TESTING.md).

```bash
yarn test:cpp:extended -- graph   # preferred
yarn test:graph                   # legacy alias
yarn test:graph:docker            # Docker → RunTests.sh (default: extended graph)
```

Narrow with `GTEST_FILTER` (legacy `GRAPH_FILTER` still mapped by `RunTestsGraph.sh`).
