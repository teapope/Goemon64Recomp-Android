
# Goemon64Recomp-Android (A1: RT64 + Vulkan)

This is a **starter Android project** that embeds the Goemon64Recomp core and targets **RT64 (Vulkan via plume)** on Android. It reuses the proven app structure from 2S2H-style ports (SDL + CMake + NDK) and is tuned for handhelds like the **AYN Thor Pro**.

## What’s included
- Android Gradle project (`Android/`) ready for Android Studio
- `SDL + Vulkan` bootstrap with a triple-buffered swapchain
- Placeholder RT64 adapter hooks (`rt64_adapter.*`)
- First-run wiring points for ROM selection / config (stubbed; integrate your UI)
- **No copyrighted game assets** are included.

## What you still need to add
1. **Upstream sources**
   - Add the Goemon64Recomp repository as a submodule at the repo root (or point the CMake to your local path):
     ```bash
     git submodule add https://github.com/klorfmorf/Goemon64Recomp Goemon64Recomp
     ```
     Ensure headers are visible under `include/` and sources under `src/`, or update `Android/app/src/main/cpp/CMakeLists.txt`.
2. **RT64 / plume**
   - If RT64 isn’t already in upstream as a submodule, add it here:
     ```bash
     git submodule add https://github.com/rt64/rt64 third_party/rt64
     ```
     Then update include/link paths under CMake.
3. **SDL2** (with Android Java + native)
   - Add SDL2 as a submodule:
     ```bash
     git submodule add https://github.com/libsdl-org/SDL third_party/SDL2
     ```
     The Gradle/CMake setup expects to build it from source with **Vulkan** enabled and use its Java Activity (`org.libsdl.app.SDLActivity`).

## Build (Android Studio)
1. Open `Android/` in **Android Studio** (SDK 34, NDK r26+ recommended).
2. Let Gradle sync. Ensure the NDK path is set (Preferences → SDK → NDK).
3. Select **app** → **assembleDebug**.
4. Install the resulting APK on your device (e.g., AYN Thor Pro).

## Run
- First run will clear the screen via Vulkan (RT64 not yet wired).
- Implement ROM selection and Goemon init in `sdl_app.cpp`.
- Wire RT64 initialization in `rt64_adapter.cpp` using your Vulkan device/surface.

## Files to fill in
- `Android/app/src/main/cpp/rt64_adapter.cpp`: call into RT64 (plume Vulkan backend) to create device resources and render a frame.
- `Android/app/src/main/assets/`: place controller DB (e.g., `recompcontrollerdb.txt`) and any shader cache seeds.
- `Android/app/src/main/java/`: you may add UI for first-run ROM picker using Storage Access Framework, or reuse SDL’s activity with native dialogs.

## Notes
- Defaults: **no MSAA**, mailbox→fifo present modes, RGBA8 SRGB format.
- Storage: prefer a root-level app folder (avoid direct writes under `Android/data` on Android 13–15).
- Performance: persist Vulkan pipeline cache under `files/rt64_cache/`.

## License
This scaffold is provided as-is; keep original licenses for upstream projects (Goemon64Recomp GPL-3.0, SDL2 zlib, etc.).
