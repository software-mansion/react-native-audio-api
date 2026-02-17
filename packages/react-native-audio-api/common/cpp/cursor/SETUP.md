`CMakeLists.txt` in this directory can be used to build the C++ side and generate `compile_commands.json` for the Cursor C++ extension.

**Generate compile_commands.json**

From this directory (`common/cpp/cursor`):

```bash
cmake -B build .
cp build/compile_commands.json ../../../../../
```

Or from the repo root:

```bash
cd packages/react-native-audio-api/common/cpp/cursor && cmake -B build . && cp build/compile_commands.json ../../../../../
```

Then reload the window or restart the C++ extension so it picks up the new `compile_commands.json` at the repo root.
