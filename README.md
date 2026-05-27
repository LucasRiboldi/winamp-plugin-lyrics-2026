<<<<<<< HEAD
<div align="center">

# 🎵 Majest Lyrics

**Automatic lyrics inside Winamp — no setup, no API key, no nonsense.**  
**Letras automáticas no Winamp — sem configuração, sem chave de API, sem complicação.**

[![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-blue?style=for-the-badge&logo=windows)](https://www.microsoft.com/windows)
[![Winamp](https://img.shields.io/badge/Winamp-5.53%2B-orange?style=for-the-badge)](https://winamp.com)
[![C++](https://img.shields.io/badge/C%2B%2B-17-purple?style=for-the-badge&logo=cplusplus)](https://isocpp.org)
[![License](https://img.shields.io/badge/license-Open%20Source-green?style=for-the-badge)](LICENSE)
[![Build](https://img.shields.io/badge/build-passing-brightgreen?style=for-the-badge)](#-building-from-source--compilando-do-zero)

---

🇺🇸 [English](#-english) · 🇧🇷 [Português](#-português)

</div>

---

## 🇺🇸 English

### ✨ What is this?

**Majest Lyrics** is a Winamp general-purpose plugin (`gen_majestlyrics.dll`) that fetches and displays song lyrics automatically whenever you hit play — right inside a resizable embedded window that docks with Winamp, follows its color theme, and stays out of your way.

No sign-ups. No API keys. No paid tiers. Just lyrics.

---

### 🚀 Features

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

### 🎵 Lyric Sources

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

### 📦 Installation (pre-built)

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

### 🔧 Building from source

**Requirements**

- **Windows 10 / 11**
- **Visual Studio 2019** Build Tools (v142 toolset) or full IDE  
  → [visualstudio.microsoft.com/downloads](https://visualstudio.microsoft.com/downloads/) → *Build Tools for Visual Studio 2019*
- No external dependencies — SDK headers are already included in `sdk/`

**Build via PowerShell**

```powershell
$msbuild = "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
& $msbuild "MajestLyrics.sln" /p:Configuration=Release /p:Platform=Win32 /v:minimal
```

Output: `Release\gen_majestlyrics.dll`

**Build via Visual Studio IDE**

1. Open `MajestLyrics.sln`
2. Set configuration to **Release | Win32**
3. **Build → Build Solution** (`Ctrl+Shift+B`)

> ⚠️ **Always build for Win32 (32-bit).** Winamp is a 32-bit application — x64 builds will not load.

---

### 📁 Project structure

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

### 🛠️ Technical notes

- Built with **C++17**, Win32 target only
- HTTP requests use the native **WinHTTP** API (TLS 1.2+, zero external DLL deps)
- Plugin type: **general-purpose** (`gen_*.dll`) — loaded by Winamp at startup
- Settings saved to `Plugins\MajestLyrics\options.txt` inside the player's Plugins folder
- Window position and size persist between sessions via Winamp's INI file

---

### 💡 Troubleshooting

**Plugin doesn't appear in the View menu**
- Make sure you copied `gen_majestlyrics.dll` to the Plugins folder and restarted the player
- Check Winamp/WACUP Preferences → Plugins to confirm it loaded

**Lyrics not found**
- Click the **Refresh** button in the lyrics window
- Some obscure tracks may not be in either database

**Window is blank**
- Play a song first — the window only populates when a track is active

---

<br>

## 🇧🇷 Português

### ✨ O que é isso?

**Majest Lyrics** é um plugin de uso geral para o Winamp (`gen_majestlyrics.dll`) que busca e exibe as letras das músicas automaticamente assim que você aperta play — dentro de uma janela embutida e redimensionável que se encaixa no Winamp, segue o tema de cores da skin e fica fora do caminho.

Sem cadastro. Sem chave de API. Sem mensalidade. Só letras.

---

### 🚀 Funcionalidades

| Recurso | Detalhes |
|---|---|
| 🎤 **Busca automática** | A letra carrega no momento em que a música começa |
| 🔄 **Cache inteligente** | Cada música é buscada uma vez por sessão — instantâneo no repeat |
| 🔁 **Botão Refresh** | Force uma nova busca a qualquer momento com um clique |
| 🎨 **Segue o tema** | As cores da janela acompanham a skin ativa do Winamp |
| 📐 **Redimensionável** | Arraste para qualquer tamanho, encaixado ou flutuante |
| 🖱️ **Scroll** | Rolagem com o scroll do mouse em letras longas (opcional) |
| ⚡ **Thread separada** | A busca acontece em segundo plano, a reprodução nunca pausa |

---

### 🎵 Fontes de letras

O Majest Lyrics tenta as fontes em ordem — se a primeira falhar, cai para a próxima automaticamente:

```
1ª  →  LRCLib       (grátis, sem chave, melhor cobertura)
2ª  →  ChartLyrics  (fallback via RapidAPI, chave inclusa)
```

| Fonte | Grátis | Chave necessária | Tipo |
|-------|--------|------------------|------|
| [LRCLib](https://lrclib.net) | ✅ | ❌ | Principal |
| [ChartLyrics](https://rapidapi.com/sridurgayadav/api/chart-lyrics) | ✅ | inclusa | Fallback |

---

### 📦 Instalação (pré-compilado)

> Não precisa compilar nada — pegue o DLL direto da pasta `dist/`.

**Passo 1 — Copie o arquivo do plugin**

```
dist/gen_majestlyrics.dll  →  C:\Program Files (x86)\Winamp\Plugins\
```

Ou para o **WACUP**:

```
dist/gen_majestlyrics.dll  →  C:\Program Files (x86)\WACUP\Plugins\
```

> 💡 O Windows vai pedir permissão de administrador ao colar em `Arquivos de Programas` — clique em **Continuar**.

**Passo 2 — Reinicie o player**

Feche e abra novamente o Winamp ou WACUP.

**Passo 3 — Abra a janela de letras**

- Barra de menu → **View** → **Majest Lyrics**
- Ou pressione **`Alt + 1`**

▶️ Toque qualquer música e as letras aparecem automaticamente!

---

### 🔧 Compilando do zero

**Requisitos**

- **Windows 10 / 11**
- **Visual Studio 2019** Build Tools (toolset v142) ou IDE completo  
  → [visualstudio.microsoft.com/downloads](https://visualstudio.microsoft.com/downloads/) → *Build Tools for Visual Studio 2019*
- Sem dependências externas — os headers do SDK já estão incluídos na pasta `sdk/`

**Compilar via PowerShell**

```powershell
$msbuild = "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
& $msbuild "MajestLyrics.sln" /p:Configuration=Release /p:Platform=Win32 /v:minimal
```

Saída: `Release\gen_majestlyrics.dll`

**Compilar pelo Visual Studio IDE**

1. Abra `MajestLyrics.sln`
2. Selecione a configuração **Release | Win32**
3. **Build → Build Solution** (`Ctrl+Shift+B`)

> ⚠️ **Sempre compile para Win32 (32 bits).** O Winamp é um aplicativo 32-bit — builds x64 não serão carregados.

---

### 📁 Estrutura do projeto

```
MajestLyrics/
├── src/
│   ├── Plugin.cpp          ← ponto de entrada do plugin, hooks IPC do Winamp
│   ├── LyricHandler.cpp    ← cache de letras e estado da sessão
│   ├── HttpClient.cpp      ← cliente HTTPS baseado em WinHTTP
│   └── EmbedWindow.cpp     ← gerenciamento da janela embutida
├── include/
│   ├── core/               ← Album, base Decoder, LyricHandler
│   ├── decoders/           ← LrcLibDecoder, ChartLyricsDecoder
│   ├── net/                ← HttpClient
│   ├── util/               ← StringUtil, codificação de URL
│   └── winamp/             ← wrappers da API do Winamp, EmbedWindow
├── res/                    ← recursos de diálogo, resource.h
├── sdk/                    ← headers do SDK do Winamp (release público do WACUP)
└── dist/                   ← gen_majestlyrics.dll pré-compilado
```

---

### 🛠️ Notas técnicas

- Compilado com **C++17**, alvo Win32 exclusivo
- Requisições HTTP usam a API nativa **WinHTTP** (TLS 1.2+, sem DLL externas)
- Tipo de plugin: **general-purpose** (`gen_*.dll`) — carregado pelo Winamp na inicialização
- Configurações salvas em `Plugins\MajestLyrics\options.txt` dentro da pasta do player
- Posição e tamanho da janela persistem entre sessões via INI do Winamp

---

### 💡 Solução de problemas

**Plugin não aparece no menu View**
- Confirme que copiou `gen_majestlyrics.dll` para a pasta Plugins e reiniciou o player
- Verifique em Winamp/WACUP Preferências → Plugins se ele foi carregado

**Letra não encontrada**
- Clique no botão **Refresh** na janela de letras
- Músicas muito obscuras podem não estar em nenhuma das bases de dados

**Janela em branco**
- Toque uma música primeiro — a janela só é preenchida com uma faixa ativa

---

<div align="center">

Feito com 🎧 para os fãs do Winamp ao redor do mundo.

</div>
=======
# winamp-plugin-lyrics-2026
Modern lyrics plugin for Winamp featuring synchronized lyrics, metadata integration and customizable display interface.
>>>>>>> 26e64547e1f8d9dfcea872aba804d68322fdb34a
