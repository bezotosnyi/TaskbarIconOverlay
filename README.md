# TaskbarIconOverlay

> Draw full custom icon overlays on individual Windows taskbar buttons, per window, with optional Win+1…Win+0 numbering. No Windhawk installation required on the target machine.

**Status: active development.** Core pipeline (injection, symbol resolution, hooking, WPF configuration UI, real-time IPC) is functional end-to-end; some edge cases are still being ironed out (see Known limitations).

> **Not to be confused with** [jlahijani/TaskbarIconOverlay](https://github.com/jlahijani/TaskbarIconOverlay), an unrelated project with the same name. That project uses the public `ITaskbarList3::SetOverlayIcon` Win32 API, which draws a small 16×16 corner badge and can't fully replace a taskbar button's icon. This project draws a full-size overlay instead – see [Why not the public overlay-icon API](#why-not-the-public-overlay-icon-api).

---

## What it does

TaskbarIconOverlay lets you assign a custom image to individual taskbar buttons – for example, distinguishing multiple windows of the same application (multiboxing, multiple game clients, multiple editor/terminal windows, etc.) – and optionally overlays a number (1–9, 0) matching the `Win+N` keyboard shortcut for that window's position.

It ships as a WPF tray application: a splash screen confirms the native engine is ready, then a configuration window lets you pick how many windows to track, assign an image to each (with live preview and drag-to-reorder), and toggle the overlay on/off. Settings persist across restarts, the app runs in the tray, and closing the window minimizes to tray rather than exiting.

## Why not the public overlay-icon API

Windows has a public API for taskbar icon badges – `ITaskbarList3::SetOverlayIcon` – but it's fixed at 16×16 pixels, drawn as a small corner badge on top of the existing icon by design, and applies per taskbar *group* rather than strictly per window when buttons are combined.

`WM_SETICON` was also tested directly and confirmed to only update Alt+Tab/title-bar icons on Windows 11, never the actual taskbar button.

There is no public, documented way to fully replace what's drawn for a taskbar button on modern Windows – this project hooks Explorer's internal rendering instead.

## How it works (high level)

```
TaskbarIconOverlay.App (WPF)          explorer.exe
┌────────────────────────┐           ┌──────────────────────────────┐
│ splash -> config UI    │   MMF     │ TaskbarIconOverlay.Engine.dll│
│ tray + single instance │──────────▶│ ├─ loads mod DLLs            │
│                        │  events   │ └─ hooks taskbar rendering   │
└──────────┬─────────────┘           └──────────────────────────────┘
           │
           │ Injector.exe
           │ CreateRemoteThread /
           │ LoadLibraryW
           │
           ▼
       inject / unload Engine.dll
```

- The native engine is injected into `explorer.exe` automatically at app startup.
- Toggling ON/OFF and changing images doesn't require re-injecting or restarting `explorer.exe` – a shared memory-mapped config and named events propagate live updates to the running mod.
- On disable/exit, hooks are cleanly removed before the engine unloads.

## Requirements

- Windows 10/11, 64-bit
- To build: Visual Studio 2026 (Desktop development with C++ workload,
  plus .NET desktop development for the WPF app), [vcpkg](https://vcpkg.io/)
  in manifest mode

## Building from source

1. Clone the repo.
2. Open `TaskbarIconOverlay.slnx` in Visual Studio. The pre-build steps automatically fetch the `taskbar-grouping` source and the required Windhawk SDK headers, so no manual dependency setup is required.
3. Build. vcpkg manifest mode restores `minhook`/`spdlog` automatically.

### Fetching third-party sources

The only third-party mod bundled with the project is `taskbar-grouping` (disables taskbar button grouping so each window gets its own button). It is GPL-3.0-licensed source from the [windhawk-mods](https://github.com/ramensoftware/windhawk-mods) project, fetched automatically by the pre-build script and pinned to a specific commit for reproducibility. It is **not** vendored or committed to this repository.

The three required Windhawk SDK headers (`windhawk_api.h`, `windhawk_api_internal.h`, `windhawk_utils.h`) are also fetched automatically during the build from a specific pinned `windhawk-mods` commit. They are placed into `src/wrapper/include/` and are **not** committed to this repository.

`taskbar-icon-overlay`, the mod that actually draws icons and numbers, is this project's own code. Its Win+N numbering behavior was originally inspired by the `taskbar-numberer` community mod, then substantially rewritten and extended with custom icon rendering and real-time configuration from the WPF app over shared memory.

## Project structure

```text
src/
├── app/                  WPF application
├── engine/               Explorer-injected engine
├── injector/             CLI injector
├── mods/
│   ├── shared/           Common mod code and API
│   ├── taskbar-grouping/ Third-party taskbar grouping mod
│   └── taskbar-icon-overlay/
│                         Project's taskbar icon overlay mod
├── wrapper/              Windhawk API compatibility layer
└── shared/
    ├── logger/           Shared spdlog-based logger
    └── symbols/          DIA-based symbol resolver

redist/                   Runtime dependencies
scripts/                  Build and deployment scripts
```

`taskbar-grouping` is fetched at build time, while the Windhawk compatibility layer is statically linked into each mod DLL to keep MinHook instances and settings storage isolated.

## Known limitations

- No installer and updater yet - build-and-run only for now.
- Real-time config propagation from the WPF app to the native mod (over a memory-mapped file) is functional but still being hardened against edge cases.
- Hooks into Explorer's internal taskbar rendering; expect breakage on some future Windows updates until symbols/hooks are updated accordingly.
- `redist/` bundles renamed Microsoft DIA/SymSrv binaries for symbol resolution - their redistribution terms haven't been independently verified yet.
- After reordering task bar items, numbering might be incorrect. This self-corrects in the next render.
- After initially enabling or updating settings: numbers don't appear until first taskbar interaction (like hovering).

## Acknowledgements

- `taskbar-grouping` is unmodified, GPL-3.0 licensed source from [windhawk-mods](https://github.com/ramensoftware/windhawk-mods) by m417z / Ramen Software, fetched at build time (see above).
- The Win+N numbering approach in `taskbar-icon-overlay` was originally inspired by the `taskbar-numberer` community mod from the same project, substantially rewritten and extended since.
- The overall symbol-hooking approach (mod lifecycle shape, signature-matching hooks) is inspired by [Windhawk](https://windhawk.eu/)'s architecture, reimplemented independently - no Windhawk installation is required on the target machine, and no Windhawk source is vendored.
- UI built with [MahApps.Metro](https://mahapps.com/). Hooking via [MinHook](https://github.com/TsudaKageyu/minhook). Logging via [spdlog](https://github.com/gabime/spdlog).

## License

**GPL-3.0, for the whole repository.**

This is a direct consequence of bundling the GPL-3.0-licensed `taskbar-grouping` mod source (see Acknowledgements) as a compiled component of this project. Its compiled DLL is a derivative work and must remain GPL-3.0 with source available. Keeping the whole project under a single license also avoids ambiguity about which parts of the repository are covered by which license.