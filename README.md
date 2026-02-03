
# Goemon64Recomp-Android (A1: RT64 + Vulkan)

Starter Android project to run **Goemon64Recomp** on Android using **RT64 (Vulkan via plume)**. Built around an SDL2 + CMake + NDK shell and tuned for handhelds like the **AYN Thor Pro**.

## Submodules expected
```bash
# SDL2 (Android Activity + native)
git submodule add https://github.com/libsdl-org/SDL third_party/SDL2
# Goemon64Recomp (upstream sources used by the Android target)
git submodule add https://github.com/klorfmorf/Goemon64Recomp Goemon64Recomp
# RT64 (if not already vendored by upstream)
git submodule add https://github.com/rt64/rt64 third_party/rt64
```

> You can also point CMake at an existing RT64 by passing `-DRT64_DIR=/absolute/or/relative/path`.

## Build (Android Studio)
1. Open `Android/` in **Android Studio** (SDK 34, NDK r26+).
2. Let Gradle sync → Build **app** → **assembleDebug**.
3. Install the APK on your device.

## What’s wired
- **SDL + Vulkan** windowing, device/swapchain, triple buffering, mailbox→fifo fallback.
- **RT64 adapter hooks** – autodetects your RT64 path and links either via its CMake target or sources-only.
- **Audio** – simple SDL_Audio stub.

## TODO for you
- Replace the **Goemon source lists** in CMake with explicit files from your upstream (avoid GLOB in production).
- Implement `RT64_InitWithVulkan/Render/Resize/Shutdown` with your RT64 headers and API calls.
- Add first‑run ROM picker (SAF) UI based on your preference.

## Defaults
- No MSAA initially. RGBA8 + SRGB, triple‑buffered.
- Pipeline cache: persist to app files dir (to be added when you hook RT64’s cache API).

## License
This scaffold is provided as-is; retain upstream licenses (Goemon64Recomp GPL-3.0, SDL2 zlib, etc.).
