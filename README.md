# TaskbarIconOverlay

> Draw full custom icon overlays on individual Windows taskbar buttons, per
> window, with optional Win+1…Win+0 numbering. No Windhawk installation
> required on the target machine.

**Status: early development / work in progress.** Architecture is settled;
implementation is ongoing.

> **Not to be confused with** [jlahijani/TaskbarIconOverlay](https://github.com/jlahijani/TaskbarIconOverlay),
> an unrelated project with the same name. That project uses the public
> `ITaskbarList3::SetOverlayIcon` Win32 API, which draws a small 16×16
> corner badge. This project draws a **full-size overlay that can cover the
> entire taskbar icon**, which that API cannot do (see
> [Why not the public overlay-icon API](#why-not-the-public-overlay-icon-api)
> below) – hence the native engine and injection approach.

---

## What it does

TaskbarIconOverlay lets you assign a custom image to individual taskbar
buttons – for example, distinguishing multiple windows of the same
application (multiboxing, multiple game clients, multiple editor/terminal
windows, etc.) – and optionally overlays a number (1–9, 0) matching the
`Win+N` keyboard shortcut for that window's position.

It ships as a single tray application with a configuration window: pick how
many windows you want to track, assign an image to each (with live preview,
drag-to-reorder), toggle on/off, done.

## Why not the public overlay-icon API

Windows does have a public API for taskbar icon badges –
`ITaskbarList3::SetOverlayIcon` – but it's fixed at 16×16 pixels and drawn as
a small corner badge on top of the existing icon, by design, not a
replacement for it. It's also a per-group (not strictly per-window) feature
when buttons are combined. Neither `SetOverlayIcon` nor `WM_SETICON` (which
we tested directly) reliably changes what's actually shown in the modern
Windows 11 taskbar – `WM_SETICON` only updates Alt+Tab / title bar icons in
practice, not the taskbar button itself. There's no public, documented way
to fully replace what's drawn for a taskbar button on Windows 11, which is
why this project hooks the taskbar's internal rendering instead.

## How it works (high level)

```
TaskbarIconOverlay.App (WPF)         explorer.exe
┌────────────────────────┐            ┌─────────────────────────┐
│ configure slots,       │  shared    │ engine.dll              │
│ ON/OFF toggle          │──memory──▶│  └─ loads 3 mod DLLs     │
│                        │  + events  │      (hooks taskbar via │
└──────────┬─────────────┘            │       MinHook)          │
           │ injector.exe             └─────────────────────────┘
           │ (CreateRemoteThread /
           │  LoadLibrary once at startup)
           ▼
     injects engine.dll into explorer.exe
```

- The native engine is injected into `explorer.exe` **once**, at app startup.
- Toggling ON/OFF and changing images does **not** re-inject or restart
  `explorer.exe` – it just updates a memory-mapped file the mod reads on
  every taskbar redraw, plus a named event for instant refresh.
- On app exit, hooks are cleanly removed before the process shuts down.

## Requirements

- Windows 10/11, 64-bit
- To build: Visual Studio 2026 (Desktop development with C++ workload, plus
  .NET desktop development for the WPF app), [vcpkg](https://vcpkg.io/) in
  manifest mode

## Building from source

No prebuilt releases yet.

1. Clone the repo.
2. Fetch the two third-party mod sources (see below) – not committed to
   this repo, downloaded on demand.
3. Open `TaskbarIconOverlay.sln` in Visual Studio, restore vcpkg
   dependencies (automatic via manifest mode / `vcpkg.json`), build.
4. Build the WPF app project the same way, or via `dotnet build`.

### Fetching the third-party mod sources

Two of the three mods bundled here (taskbar grouping disable, taskbar
thumbnail reorder) are **not vendored in this repository** – they're
GPL-3.0 licensed sources from the
[windhawk-mods](https://github.com/ramensoftware/windhawk-mods) project,
downloaded at build time and pinned to a specific commit for
reproducibility:

```
https://raw.githubusercontent.com/ramensoftware/windhawk-mods/<pinned-commit>/mods/taskbar-grouping.wh.cpp
https://raw.githubusercontent.com/ramensoftware/windhawk-mods/<pinned-commit>/mods/taskbar-thumbnail-reorder.wh.cpp
```

See `scripts/fetch-third-party-mods.ps1` for the exact pinned commit and
download logic. These files are gitignored – nothing from that repo is
checked into this one.

You'll also need 3 SDK headers (`mods_api.h`, `mods_api_internal.h`,
`windhawk_utils.h`) copied from a Windhawk portable install into
`src/mods/third_party/` – see the README there for exact instructions.

## Project structure

```
src/
├── engine/     # native shim DLL: implements the Wh_* API surface our
│               # mods link against (logging, settings, hooks, symbols)
├── injector/   # injector.exe - CLI: enable / disable / status
├── mods/       # the 3 mods themselves (numbering - original; grouping-
│               # disable and thumbnail-reorder - fetched, see above),
│               # each with a thin export shim
└── app/        # WPF configuration UI + tray
```

## Known limitations

- Numbering overlays are capped at 10 slots (`Win+1`…`Win+0` – there's
  nowhere else for a number to map to). Image overlays are not capped there.
- No installer yet – build-and-run only for now.
- No live unhook on toggle-off is exposed yet beyond the shared "enabled"
  flag; a full explorer.exe restart is only used if injection itself needs
  to be redone.
- Hooks into Explorer's internal taskbar rendering; expect breakage on some
  future Windows updates until symbols/hooks are updated accordingly.

## Acknowledgements

Two of the three bundled mods – taskbar grouping disable and taskbar
thumbnail reorder – are unmodified sources from
[windhawk-mods](https://github.com/ramensoftware/windhawk-mods) by m417z /
Ramen Software, fetched at build time (see above), wrapped with a thin
custom export shim so they can be loaded by this project's own engine
instead of Windhawk itself. Both are explicitly GPL-3.0 licensed by their
author – see [License](#license).

The overall approach (mod lifecycle shape, symbol-hooking via
signature-matching, settings API shape) is inspired by
[Windhawk](https://windhawk.eu/)'s architecture, reimplemented independently
for this project's own minimal native engine – no Windhawk source is
vendored or required on the target machine.

## License

**GPL-3.0**, for the whole repository.

This is a direct consequence of bundling two GPL-3.0-licensed mod sources
(see Acknowledgements) as compiled components of this project – their
compiled DLLs are derivative works and must remain GPL-3.0 with source
available, and keeping the whole project under one license avoids any
ambiguity about what's covered by what.