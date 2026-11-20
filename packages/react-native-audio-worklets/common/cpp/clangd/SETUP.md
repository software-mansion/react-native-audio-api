`CMakeLists.txt` in this directory generates `compile_commands.json` for clangd
(any editor with a clangd integration) and for `pnpm lint:cpp` (clang-tidy).

**Generate compile_commands.json**

From the monorepo root, after `pnpm install`:

```bash
pnpm setup:clangd
```

That runs this directory's `generate-and-copy.sh` (after audio-api's). It runs
React Native codegen into `build/codegen` for the TurboModule spec header,
configures CMake into `build/`, copies the database to the package root and
merges it into the repo-root `compile_commands.json`.

**Optional**

For full IntelliSense on `NativeAudioWorkletsModule.cpp`, run `pod install` in
`apps/fabric-example/ios`: it needs folly from `Pods/Headers/Public`. Without
Pods the file shows unresolved includes, and `pnpm lint:cpp` skips it.
