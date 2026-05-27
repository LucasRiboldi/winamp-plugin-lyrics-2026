<div align="center">

# 🎵 Majest Lyrics

**Automatic lyrics inside Winamp — no setup, no API key, no nonsense.**

[![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-blue?style=for-the-badge&logo=windows)](https://www.microsoft.com/windows)
[![Winamp](https://img.shields.io/badge/Winamp-5.53%2B-orange?style=for-the-badge)](https://winamp.com)
[![C++](https://img.shields.io/badge/C%2B%2B-17-purple?style=for-the-badge&logo=cplusplus)](https://isocpp.org)
[![License](https://img.shields.io/badge/license-Open%20Source-green?style=for-the-badge)](LICENSE)
[![Build](https://img.shields.io/badge/build-passing-brightgreen?style=for-the-badge)](#-building-from-source)

</div>

---

## ✨ What is this?

**Majest Lyrics** is a Winamp general-purpose plugin (`gen_majestlyrics.dll`) that fetches and displays song lyrics automatically whenever you hit play — right inside a resizable embedded window that docks with Winamp, follows its color theme, and stays out of your way.

No sign-ups. No API keys. No paid tiers. Just lyrics.

---

## 🚀 Features

| Feature | Details |
|---|---|
| 🎤 **Auto-fetch** | Lyrics load the moment a song starts playing |
| 🔄 **Smart cache** | Each song fetched once per session — instant on repeat |
| 🔁 **Refresh button** | Force a new fetch anytime with one click |
| 🎨 **Theme-aware** | Window colors follow Winamp's active skin |
| 📐 **Resizable** | Drag to any size, docked or floating |
| 🖱️ **Scrolling** | Optional mouse-wheel scroll through long lyrics |
| ⚡ **Threaded** | Fetching happens in the background, playback never pauses |

---

## 🎵 Lyric Sources

Majest Lyrics tries sources in order — if the first one fails, it falls back automatically:

```
1st  →  LRCLib       (free, no key, best coverage)
2nd  →  ChartLyrics  (fallback via RapidAPI, bundled key)
```

| Source | Free | Key required | Type |
|--------|------|--------------|------|
| [LRCLib](https://lrclib.net) | ✅ | ❌ | Primary |
| [ChartLyrics](https://rapidapi.com/sridurgayadav/api/chart-lyrics) | ✅ | bundled | Fallback |

---

## 📦 Installation (pre-built)

> No need to compile anything — grab the DLL from the `dist/` folder.

**Step 1 — Copy the plugin file**

```
dist/gen_majestlyrics.dll  →  C:\Program Files (x86)\Winamp\Plugins\
```

Or for **WACUP**:

```
dist/gen_majestlyrics.dll  →  C:\Program Files (x86)\WACUP\Plugins\
```

> 💡 Windows will ask for administrator permission when pasting into `Program Files` — click **Continue**.

**Step 2 — Restart your player**

Close and reopen Winamp or WACUP.

**Step 3 — Open the lyrics window**

- Menu bar → **View** → **Majest Lyrics**
- Or press **`Alt + 1`**

▶️ Play any song and lyrics will appear automatically!

---

## 🔧 Building from source

### Requirements

- **Windows 10 / 11**
- **Visual Studio 2019** Build Tools (v142 toolset) or full IDE  
  → Download: [visualstudio.microsoft.com/downloads](https://visualstudio.microsoft.com/downloads/) → *Build Tools for Visual Studio 2019*
- No external dependencies — SDK headers are already included in `sdk/`

### Build (command line)

Open **PowerShell** and run:

```powershell
$msbuild = "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
& $msbuild "MajestLyrics.sln" /p:Configuration=Release /p:Platform=Win32 /v:minimal
```

Output: `Release\gen_majestlyrics.dll`

### Build (Visual Studio IDE)

1. Open `MajestLyrics.sln`
2. Set configuration to **Release | Win32**
3. **Build → Build Solution** (`Ctrl+Shift+B`)

> ⚠️ **Always build for Win32 (32-bit).** Winamp is a 32-bit application — x64 builds will not load.

---

## 📁 Project structure

```
MajestLyrics/
├── src/
│   ├── Plugin.cpp          ← plugin entry point, Winamp IPC hooks
│   ├── LyricHandler.cpp    ← lyric cache and session state
│   ├── HttpClient.cpp      ← WinHTTP-based HTTPS client
│   └── EmbedWindow.cpp     ← embedded window management
├── include/
│   ├── core/               ← Album, Decoder base, LyricHandler
│   ├── decoders/           ← LrcLibDecoder, ChartLyricsDecoder
│   ├── net/                ← HttpClient
│   ├── util/               ← StringUtil, URL encoding
│   └── winamp/             ← Winamp API wrappers, EmbedWindow
├── res/                    ← dialog resources, resource.h
├── sdk/                    ← Winamp SDK headers (WACUP public release)
└── dist/                   ← pre-built gen_majestlyrics.dll
```

---

## 🛠️ Technical notes

- Built with **C++17**, Win32 target only
- HTTP requests use the native **WinHTTP** API (TLS 1.2+, zero external DLL deps)
- Plugin type: **general-purpose** (`gen_*.dll`) — loaded by Winamp at startup
- Settings saved to `Plugins\MajestLyrics\options.txt` relative to the Winamp folder
- Window position and size persist between sessions via Winamp's INI file

---

## 💡 Troubleshooting

**Plugin doesn't appear in the View menu**
- Make sure you copied `gen_majestlyrics.dll` (not the old `gen_lyrics.dll`) to the Plugins folder
- Restart the player after copying
- Check Winamp/WACUP Preferences → Plugins to confirm it loaded

**Lyrics not found**
- Click the **Refresh** button in the lyrics window
- Some obscure tracks may not be in either database

**Window is blank / white**
- Play a song first — the window only populates when a track is active

---

<div align="center">

Made with 🎧 for Winamp fans everywhere.

</div>
