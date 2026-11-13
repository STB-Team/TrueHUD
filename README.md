## Requirements
* [CMake](https://cmake.org/)
	* Add this to your `PATH`
* [The Elder Scrolls V: Skyrim Special Edition](https://store.steampowered.com/app/489830)
	* This STB build targets Skyrim SE 1.5.97 only; installation is explicit, not a post-build side effect.
* [Vcpkg](https://github.com/microsoft/vcpkg)
	* Add the environment variable `VCPKG_ROOT` with the value as the path to the folder containing vcpkg
* Visual Studio Community 2026 (v145)
	* Desktop development with C++
* CommonLibSSE-NG is pinned as the `extern/CommonLibSSE-NG` submodule.

## User Requirements
* [Address Library for SKSE](https://www.nexusmods.com/skyrimspecialedition/mods/32444)

## Building
```
git submodule update --init --recursive
cmake --preset default
cmake --build build --config Release
ctest --test-dir build -C Release -R "^TrueHUDUpdateCache$" --output-on-failure
```

## STB optimization build

The supplied STB C++ sources are imported separately from upstream Flash sources.
Keep the installed STB SWFs: upstream FLA/AS files are not replacements for them.

- Skip identical accepted NPC/boss data, special-resource and player-penalty packets.
- Cache owned text, never pointers into the periodically refreshed actor data.
- Invalidate on widget reconfiguration; do not cache calls made before deferred Flash initialization.
- Skip identical NPC display transforms, without reducing world projection or resource sampling frequency.
- Clear debug graphics once after the last primitive, not on every subsequent empty frame.
- Retain the player's regular data updates: they refresh visibility timers and animated layout.
- A missing actor position/3D hides the bar instead of projecting an uninitialized point.

`TRUEHUD_PROFILE=OFF` is the default: optimizations are always enabled, without profiling.
For a diagnostic build, configure with `cmake --preset default -DTRUEHUD_PROFILE=ON`.
It alternates 10-second `baseline`/`optimized` windows automatically and writes `TrueHUDPerf:` to
`SKSE/TrueHUD.log`. Baseline bypasses the new skip guards in the same instrumented DLL; it is not
the previously installed binary. The missing-position correctness fix is active in both modes.
Timers measure nested elapsed time, not CPU time or GPU FPS; do not add inclusive phases.
Each phase reports per-frame mean/p95/p99/max. Percentiles retain at most 4096 frames per window;
overflow is reported. Actor details retain 32 IDs per window, first/last states and health, plus
transition counts; they are not a complete event trace. `report_ms` exposes most log-output overhead.
The first window includes initialization and is not a steady-state performance sample.

Before installation, preserve the previous DLL/PDB pair, stop Skyrim, and verify the new pair's
CodeView GUID/age and the unchanged public widget ABI. Build outputs are in `build/src/Release`.

## License

[GPL-3.0-or-later](COPYING) WITH [Modding Exception AND GPL-3.0 Linking Exception (with Corresponding Source)](EXCEPTIONS). Specifically, the Modded Code is Skyrim (and its variants) and Modding Libraries include [SKSE](https://skse.silverlock.org/) and Commonlib (and variants).
