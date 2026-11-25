# Start Pos Anywhere

A [Geode](https://geode-sdk.org) mod for Geometry Dash 2.2091 (Windows): start a level from any percentage, using start positions built from your own runs.

See [about.md](about.md) for what the mod does and [changelog.md](changelog.md) for versions.

## Building

Requirements: Geode SDK v6.0.0, the Geode CLI, CMake and Visual Studio 2026 (or MSVC 19.44+).

```sh
geode build
```

The packaged `.geode` file is installed into the Geometry Dash profile set up with `geode profile add`. Every push also builds the mod through GitHub Actions (see the workflow's artifacts).

## Code layout

| File | Role |
|---|---|
| `src/main.cpp` | Hooks: recording, spawning, pause menu buttons, manual start positions, keys |
| `src/Snapshot.hpp` | What a snapshot stores and how it is read from the player |
| `src/Storage.cpp` | Per-level JSON save files |
| `src/SpawnPopup.cpp` | The "spawn at %" popup with its slider |
