# Licensing & Intellectual Property Compliance

**Project:** Bangla Lekhi  
**Document:** `docs/LICENSING.md`  
**Date:** 2026-10-24  
**Author:** Lead Linux Desktop & Input-Method Engineer  
**Status:** Completed (Milestone 0)

---

## 1. Project License

**Bangla Lekhi** is licensed under the **GNU General Public License, Version 3.0 or later (GPL-3.0-or-later)**.

```text
Bangla Lekhi — Native Bengali Phonetic Input Method for Linux
Copyright (C) 2026 Bangla Lekhi Contributors

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
```

---

## 2. Third-Party Code & Dependency Provenance

| Component / Crate | Upstream Origin | Upstream License | Usage in Bangla Lekhi | Compatibility Status |
|---|---|---|---|---|
| **`riti`** | [OpenBangla/riti](https://github.com/OpenBangla/riti) (Muhammad Mominul Huque) | **MPL-2.0** | Core phonetic transliteration and candidate suggestion logic | Compatible as Larger Work (MPL §3.3) |
| **`okkhor`** | [OpenBangla/okkhor](https://github.com/OpenBangla/okkhor) (Muhammad Mominul Huque) | **MPL-2.0** | Avro phonetic parsing and mapping engine | Compatible |
| **`upodesh`** | [OpenBangla/upodesh](https://github.com/OpenBangla/upodesh) (Muhammad Mominul Huque) | **MPL-2.0** | Finite State Transducer (FST) word suggestion library | Compatible |
| **`poriborton`** | [OpenBangla/poriborton](https://github.com/OpenBangla/poriborton) | **MPL-2.0** | Unicode / ANSI charset conversion | Compatible |
| **`ahash`** | AHash contributors | MIT / Apache-2.0 | Fast, DoS-resistant hashing | Compatible |
| **`serde_json`** | Serde contributors | MIT / Apache-2.0 | Serialization for configuration and dictionaries | Compatible |
| **`regex`** | Rust Project | MIT / Apache-2.0 | Regular expressions for rule patterns | Compatible |
| **`libibus-1.0`** | The IBus Project | LGPL-2.1-or-later | Linux input method framework shared library | Compatible |
| **`libgtk-4`** | The GNOME Project | LGPL-2.1-or-later | Graphical user interface toolkit | Compatible |
| **`libadwaita-1`** | The GNOME Project | LGPL-2.1-or-later | GNOME design building blocks | Compatible |

---

## 3. Analysis of MPL-2.0 and GPL-3.0 Compatibility

The core engine reused from OpenBangla (`riti`, `okkhor`, `upodesh`) is licensed under the **Mozilla Public License 2.0 (MPL-2.0)**. 

### 3.1 Legal Mechanism: "Larger Work" (Section 3.3)
Section 3.3 of the MPL-2.0 explicitly grants permission to combine MPL-2.0 Covered Software into a Larger Work with software governed by one or more Secondary Licenses:
> *"You may create a Larger Work by combining Covered Software with other code governed by the terms of one or more Secondary Licenses, and distribute the Larger Work under the terms of such Secondary License(s)..."*

Exhibit A of MPL-2.0 lists the designated **Secondary Licenses**, which explicitly include:
- GNU General Public License, Version 2.0 or later
- GNU General Public License, Version 3.0 or later
- GNU Lesser General Public License, Version 2.1 or later
- GNU Affero General Public License, Version 3.0 or later

### 3.2 Compliance Checklist for Bangla Lekhi
To maintain 100% legal compliance:
1. **File-Level Isolation:** All source files originating from `riti` and related MPL crates will retain their original MPL-2.0 copyright notices and header blocks.
2. **Source Code Availability:** If any modification is made directly to an MPL-2.0 covered file, that file's modified source code will be made available under MPL-2.0.
3. **Larger Work Licensing:** The combined work (the IBus engine, the settings UI, the glue code, and the final compiled `.deb` package) is distributed under **GPL-3.0-or-later**.
4. **Notice Preservation:** All upstream copyright notices for Muhammad Mominul Huque and OpenBangla contributors are preserved in full inside `NOTICE` and `debian/copyright`.

---

## 4. Phonetic Scheme & Linguistic Rule Provenance

- **Avro Phonetic Scheme:**
  - The Avro Phonetic scheme rules (e.g., `ami` -> আমি, `kkh` -> ক্ষ) were originally designed and published by Dr. Mehdi Hasan Khan (OmicronLab).
  - The phonetic grammar rules describe a linguistic transliteration mapping scheme.
  - **Compliance Rule:** No proprietary Windows Avro binary code (`Avro.exe`), Windows DLLs, proprietary sound files, or installer assets have been copied or decompiled. All logic is executed via the open-source Rust implementation (`okkhor` / `riti`).

---

## 5. Lexicon & Dictionary Data Provenance

- **System Dictionary (`dictionary.json`):**
  - Compiled Bengali word frequency and spelling lexicon derived from open Bengali linguistic databases and OpenBangla data files.
  - The dictionary dataset is licensed under open, GPL-compatible terms.
- **Autocorrect Data (`autocorrect.json`):**
  - Common typographical error mappings for Bengali phonetic typing.
- **Privacy & Isolation:**
  - User personal dictionaries are stored separately in the user's `$XDG_DATA_HOME` (`~/.local/share/bangla-lekhi/`) and are never merged into system files or transmitted externally.

---

## 6. Trademarks, Logos, and Branding

- **Project Identity:** The project is named **Bangla Lekhi** to distinguish it cleanly from OpenBangla Keyboard and OmicronLab's Avro Keyboard.
- **Original Artwork:**
  - Bangla Lekhi uses entirely newly designed, original SVG vector icons for the application launcher, desktop panel indicator, and settings window.
  - No trademarked logos or artwork from Windows Avro or OpenBangla Keyboard are included in the repository or packaging.

---

## 7. Required Notices & Attributions

The following notice will be included in the project root (`NOTICE`) and in the Debian package copyright file (`debian/copyright`):

```text
This project incorporates software developed by:
1. Muhammad Mominul Huque and OpenBangla contributors
   - Project: OpenBangla riti, okkhor, and upodesh libraries
   - License: MPL-2.0 (Mozilla Public License 2.0)
   - Source: https://github.com/OpenBangla/riti

2. OmicronLab / Dr. Mehdi Hasan Khan
   - Origination and standardization of the Avro Phonetic scheme
```
