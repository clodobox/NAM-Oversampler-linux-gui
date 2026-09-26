# iPlug2 Linux GUI overrides

The `iPlug2` submodule (`DLC86/iPlug2`) doesn't yet have Linux GUI support, and this
repo doesn't have push access to patch it there. These 18 files are locally patched
copies of the iPlug2 core files needed to enable the NanoVG/X11 Linux VST3 editor
(the approach mirrors [iPlug2/iPlug2#1336](https://github.com/iPlug2/iPlug2/pull/1336)):

- `IGraphics/Platforms/IGraphicsLinux.cpp`/`.h` — X11 window embedding, GL context,
  event pump.
- `IPlug/VST3/IPlugVST3_View.h` — `kPlatformTypeX11EmbedWindowID` support.
- `IPlug/IPlugTimer.cpp`/`.h` — a real pthread-based timer for Linux (was a no-op
  stub for the previous headless build).
- `IPlug/IPlugPaths.cpp` — `LocateResource()` now actually resolves bundled
  fonts/images/SVGs relative to the plugin's `.so` (was a stub).
- The rest are small, mostly additive changes (a `FONT_DESCRIPTOR_TYPE` for Linux,
  an `OS_LINUX` branch in a couple of `#if defined(OS_...)` chains, a GfxMutex for
  thread-safety, two portability fixes unrelated to Linux specifically) needed for
  the above to compile.

`NeuralAmpModeler/projects/linux-vst3/CMakeLists.txt` copies this tree directly over
the `iPlug2` submodule's checkout at CMake configure time (`file(COPY ...)`), rather
than shadowing it via a second include path — the latter doesn't work, because
unmodified iPlug2 files living in the *same* directory as a patched file
quote-include their sibling, which the compiler resolves before any `-I` search path,
so they'd still find the unmodified original and redefine everything.

This never touches the submodule's git state (only its on-disk working tree), so
`iPlug2` stays pinned at its usual commit. If `DLC86/iPlug2` ever gains its own
Linux GUI support, or this repo gets push access to contribute it upstream, this
directory can be deleted and the CMakeLists.txt `file(COPY ...)` step removed.
