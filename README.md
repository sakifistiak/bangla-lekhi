# Bangla Lekhi — বাংলা লেখি

**Native Bengali phonetic input method for Linux**

[![License: GPL-3.0-or-later](https://img.shields.io/badge/License-GPL--3.0--or--later-blue.svg)](LICENSE)
[![Platform: Ubuntu 26.04 LTS](https://img.shields.io/badge/Platform-Ubuntu%2026.04%20LTS-E95420.svg)](https://ubuntu.com)

Bangla Lekhi is a fast, native IBus input method engine for typing Bengali (Bangla) on Linux using Avro-compatible phonetic transliteration — without any internet connection or proprietary components.

---

## Features

| Feature | Details |
|---|---|
| **Phonetic Layout** | Avro-compatible (same as Windows Avro Keyboard) |
| **Engine** | Rust core + C IBus daemon — ~3 MB RAM |
| **Suggestions** | FST-powered Bengali candidates plus common-word prefix suggestions |
| **Clipboard History** | In-memory history from desktop clipboard changes (up to nine entries) |
| **Toggle Key** | F12 (configurable via Settings) |
| **Settings UI** | GTK4 / Libadwaita native app |
| **Offline** | 100% offline, no telemetry |
| **License** | GPL-3.0-or-later |

---

## Quick Start

### Install from .deb

```sh
sudo dpkg -i bangla-lekhi_1.0.0_amd64.deb
sudo apt-get install -f   # fix any missing deps
```

### Activate

1. **Settings → Keyboard → Input Sources → +**
2. Search for **Bangla Lekhi** and add it
3. Press **F12** to toggle between English and Bangla

### Settings App

```sh
bangla-lekhi-settings
```

---

## Phonetic Typing Reference

| Type | Get | Type | Get |
|------|-----|------|-----|
| `ami` | আমি | `tumi` | তুমি |
| `bangla` | বাংলা | `bangladesh` | বাংলাদেশ |
| `bhalobashi` | ভালোবাসি | `kemon` | কেমন |
| `t` | ত | `T` | ট |
| `d` | দ | `D` | ড |
| `ng` | ং | `Ng` | ঙ |
| `gg` | জ্ঞ | `kkh` | ক্ষ |
| `jny` | জন্য | `t\`` | ৎ |
| `.` | । | `0`–`9` | ০–৯ |

When a common word prefix is typed, candidates are also shown (for example, `b`
offers `ব`, `বাংলা`, `বাংলাদেশ`, and other common words; `bi` offers `বি`,
`বিয়ে`, and `বিড়াল`). Use Up/Down and Enter, or click a candidate in the
lookup table. Number keys type Bengali digits (for example, `1` produces `১`)
rather than selecting a candidate.

Press `Ctrl+Shift+V` to open clipboard history and select an item to paste.
History keeps up to nine clipboard text entries in memory until the IBus engine
exits. The engine reads the desktop clipboard while running; password/private
input fields are excluded when the application marks them as sensitive.

Choose the mode toggle in **Bangla Lekhi Settings → General → Toggle Key**.
`Ctrl+Space` can conflict with IBus or GNOME's input-source shortcut; disable
that system shortcut if you select it.

---

## Building from Source

### Prerequisites

```sh
sudo apt install cargo rustc libibus-1.0-dev python3-gi gir1.2-gtk-4.0 gir1.2-adw-1
```

### Build

```sh
# Clone and build
git clone <repo>
cd bangla-lekhi

# Set GnuTLS workaround (if needed on your system)
export GNUTLS_SYSTEM_PRIORITY_FILE="$HOME/.config/gnutls.conf"

make all          # build engine + settings app
make test         # run all 100+ tests
make deb          # build .deb package
```

### Local Testing (no install needed)

```sh
make register-local   # builds, installs, refreshes IBus registry, and restarts IBus
```

---

## Architecture

```
┌─────────────────────────────────────────────┐
│  Application (GTK / Qt / Browser / Electron) │
└──────────────────┬──────────────────────────┘
                   │ IBus IPC (D-Bus)
┌──────────────────▼──────────────────────────┐
│  bangla-lekhi-engine  (C / GLib / IBus API)  │
│  ┌────────────────────────────────────────┐  │
│  │  libbangla_lekhi_core.a  (Rust)        │  │
│  │  ├── phonetic.rs  (okkhor parser)      │  │
│  │  ├── suggest.rs   (upodesh FST)        │  │
│  │  ├── state.rs     (mode/composition)   │  │
│  │  └── config.rs    (settings.json)      │  │
│  └────────────────────────────────────────┘  │
└─────────────────────────────────────────────┘
```

---

## Configuration

Settings are stored in `~/.config/bangla-lekhi/settings.json`:

```json
{
  "toggle_key": "F12",
  "autocorrect": true,
  "show_candidates": true,
  "candidate_count": 5
}
```

Edit via GUI: `bangla-lekhi-settings`

---

## Troubleshooting

**Bangla Lekhi not appearing in Input Sources?**
```sh
ibus write-cache
ibus restart
```

**Candidates not showing?**
- Make sure IBus is running: `ibus-daemon -drx`
- Check GNOME IBus integration: Settings → Keyboard → Input Sources

**IBus not found?**
```sh
sudo apt install ibus ibus-gtk4
```

See [`docs/TROUBLESHOOTING.md`](docs/TROUBLESHOOTING.md) for more.

---

## License

GPL-3.0-or-later — see [LICENSE](LICENSE).

The `okkhor` and `upodesh` crates are licensed under MPL-2.0.
