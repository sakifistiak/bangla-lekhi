# Bangla Lekhi
## Complete Product Specification & Implementation Plan

**Project:** Bangla Lekhi  
**Platform:** Ubuntu 26.04 LTS, GNOME 50, Wayland-first  
**Product type:** Native Bengali phonetic input method for Linux  
**Reference experience:** Windows Avro Keyboard  
**Primary language:** English UI; Bengali and English typing  
**Delivery:** Complete source code + installable `.deb` package  
**Development environment:** Google Antigravity  
**Document status:** Engineering specification v2 (owner decisions included)

---

# PART 0 — OWNER DECISIONS (ALREADY MADE, DO NOT RE-LITIGATE)

These decisions were made by the project owner. Follow them unless research proves one is technically impossible; in that case, stop and explain before changing anything.

1. **Target machine:** The machine you are running on is the real target: Ubuntu 26.04 with GNOME 50. Verify this first (`lsb_release -a`, `gnome-shell --version`, `echo $XDG_SESSION_TYPE`, `ibus version`). All "tested" claims must come from this real environment. If you are running in a sandbox/VM that does not match, say so explicitly and mark affected tests as "not verified".
2. **License:** The project is licensed **GPL-3.0-or-later**. Reusing GPL-compatible code is therefore allowed, provided every notice and attribution is preserved.
3. **Engine reuse:** Evaluate and prefer reusing the **OpenBangla Keyboard `riti` Rust engine** (Avro-compatible phonetic core) as the transliteration core, instead of rewriting it from scratch. Before reusing, verify its exact license, version, API stability, and whether it is maintained. Record this in `docs/LICENSING.md`. If it turns out unsuitable, fall back to a clean implementation and explain why.
4. **Stack:** The owner wants the option that is **best and lightweight**. Default direction, to be confirmed or overturned by research with reasons:
   - Core transliteration: Rust (`riti`) exposed to the IBus layer through a small C-compatible interface.
   - IBus engine: a small native process (C or Rust with libibus/GLib bindings), whichever is smaller, more stable, and easier to package on Ubuntu 26.04. A Python/PyGObject prototype is acceptable only as a throwaway spike, not as the final engine, unless measurements show it is lightweight enough.
   - Settings app: GTK4 (optionally libadwaita) to match GNOME. No Electron, no web runtime.
   - Prefer few dependencies, low idle CPU and RAM, and fast startup.
5. **Workflow:** Plan first, implement later. Stop for owner review after the research/plan phase.

---

# PART 1 — PROJECT OVERVIEW

## 1. Product vision

Build **Bangla Lekhi**, a reliable, polished, native Linux Bengali phonetic keyboard that recreates as much of the important Windows Avro Keyboard experience as practical.

This must not be merely a transliteration website, a standalone text editor, or a keyboard demo. It must be a real Linux input method that allows users to type Bengali in supported desktop applications.

The intended experience should feel familiar to someone who already uses Windows Avro Keyboard.

### Core user experience

- A persistent, accessible keyboard/input indicator in the GNOME desktop.
- A global keyboard shortcut to switch between English and Bangla.
- Avro-compatible phonetic typing rules.
- Live composition and candidate suggestions.
- Appropriate Bengali Unicode output.
- Settings and preferences accessible from the desktop.
- Automatic startup after login, configurable by the user.
- Reliable operation across supported GTK, Qt, browser and Electron applications.
- Easy installation and removal using a Debian package.
- A maintainable source repository with documentation and automated tests.

## 2. Target environment

The primary target is:

- Ubuntu 26.04 LTS.
- GNOME 50.
- Wayland session, with X11 compatibility investigated where feasible.
- IBus input-method framework.
- GTK and Qt applications, browsers, editors and Electron-based applications.

Do not assume that documentation written for GNOME 46 accurately describes GNOME 50. Verify the installed GNOME, IBus, GTK, Qt and packaging APIs against the actual target environment.

GNOME 46+ compatibility is a design consideration, but Ubuntu 26.04 GNOME 50 is the primary acceptance target.

## 3. Engineering principles

1. Native input-method integration takes priority over visual imitation.
2. Correct text composition takes priority over extra features.
3. The keyboard must not require root privileges for normal operation.
4. User data and typing history must remain private.
5. All dependencies, phonetic rules, dictionaries and reused code must have reviewed licenses.
6. Prefer stable system APIs and maintainable code.
7. Do not claim that a feature works until it has been tested.
8. Do not silently replace the user's existing input-method configuration.
9. Build in small, reviewable, testable milestones.
10. Document every known limitation.
11. Keep the footprint small: low memory, low idle CPU, fast startup.

---

# PART 2 — REQUIRED FEATURES

## 4. Essential features — P0

### 4.1 Avro-compatible phonetic typing

Implement (or reuse, per Part 0) a Bengali phonetic engine whose observable typing behavior is compatible with the established Avro Phonetic scheme.

Examples of expected basic conversions:

| Latin input | Expected Bengali output |
|---|---|
| `ami` | আমি |
| `bangla` | বাংলা |
| `bangladesh` | বাংলাদেশ |
| `kemon acho` | কেমন আছো |
| `bhalobashi` | ভালোবাসি |

These are smoke-test examples, not a complete specification of Avro's rules.

The implementation must investigate and test:

- Vowel mappings.
- Consonant mappings.
- Multi-character sequences.
- Aspirated consonants.
- Conjunct formation.
- Hasanta/virama behavior.
- Kar and vowel-sign composition.
- Reph and relevant Bengali orthographic behavior.
- Ambiguous sequences and rule precedence.
- Punctuation and digits.
- Latin text preservation.
- Word boundaries and spaces.
- Backspace and deletion during composition.
- Unicode normalization where applicable.
- Candidate selection and commit behavior.

Create a dedicated phonetic rule test corpus. Do not assume that a small dictionary of replacements is sufficient.

The engine should be deterministic where the underlying scheme specifies deterministic behavior. Dictionary-assisted prediction must be a separate layer from core transliteration.

### 4.2 English/Bangla mode switching

Provide a configurable global shortcut for switching between English and Bangla input.

Requirements:

- Choose a practical default shortcut after checking IBus and GNOME conflicts.
- Allow users to customize it.
- Clearly display the current mode.
- Support switching without opening the settings window.
- Preserve the user's chosen mode according to documented behavior.
- Avoid intercepting shortcuts when Bangla Lekhi is inactive.
- Document shortcut conflicts and recovery procedures.

Do not assume that a global keyboard shortcut implemented through a normal desktop window will work reliably in every Wayland application. Prefer appropriate IBus input-method APIs and documented desktop integration.

### 4.3 System-wide input

Bangla Lekhi must function as an IBus input engine, not merely as a background process that simulates keystrokes.

Test text entry in:

- GNOME Text Editor.
- Firefox.
- Chromium-based browsers.
- VS Code.
- LibreOffice Writer.
- A GTK application.
- A Qt application.
- A terminal emulator, with clearly documented limitations.
- Electron applications where applicable.

Support may vary between application toolkits and input-field implementations. Document any exceptions instead of claiming universal compatibility.

### 4.4 Composition and candidate selection

Implement correct pre-edit and commit behavior.

Requirements:

- Display in-progress phonetic composition appropriately.
- Display candidate suggestions where supported.
- Support candidate navigation and selection using keyboard controls.
- Support Escape to cancel or dismiss composition where appropriate.
- Support Backspace during composition.
- Handle Enter, Space, punctuation and word boundaries consistently.
- Avoid duplicate characters, dropped text and accidental commits.
- Preserve cursor and surrounding text behavior to the extent supported by IBus.

**Known risk to investigate early:** on Wayland, the candidate window may be positioned incorrectly or not at all in some applications (especially some Electron/Qt/terminal cases). Test this in Milestone 2, record exact behavior per application, and design a fallback (for example, showing the best candidate inline in pre-edit).

### 4.5 Settings interface

Create an English-language settings application.

Minimum settings:

- Enable/disable Bangla Lekhi.
- Choose Bangla or English input mode.
- Configure the toggle shortcut.
- Configure candidate suggestion behavior.
- Enable or disable AutoCorrect.
- Enable or disable user dictionary functionality.
- Enable or disable launch-at-login.
- Reset settings to defaults.
- Display application version and diagnostic information.
- Provide a help/troubleshooting section.

Settings must persist across restarts.

### 4.6 Desktop indicator

Provide a discoverable desktop control for the input method.

Requirements:

- Display the current input mode.
- Offer a way to access settings.
- Offer a way to switch modes.
- Remain synchronized with the actual IBus engine state.
- Avoid creating multiple indicators after restarting.
- Recover sensibly if the engine crashes or IBus restarts.

Investigate the native IBus panel/status integration and Ubuntu's available AppIndicator support before choosing the implementation.

Do not make a GNOME Shell extension a mandatory dependency for core typing functionality.

If an independent top-bar indicator cannot be delivered reliably on the target desktop without an extension, document the limitation and use the standard GNOME/IBus input-source indicator as the reliable fallback.

---

# PART 3 — AVRO-LIKE ADVANCED FEATURES

## 5. Important features — P1

### 5.1 Dictionary-based suggestions

Implement a local dictionary-assisted candidate system.

Requirements:

- Suggest plausible Bengali words based on phonetic input.
- Rank candidates consistently.
- Support keyboard navigation.
- Allow users to accept or dismiss suggestions.
- Keep the core phonetic engine usable without a dictionary.
- Work offline.
- Avoid sending typed text to external services.

Measure suggestion latency on representative inputs. Avoid blocking the main input-processing path with expensive dictionary lookups.

### 5.2 AutoCorrect

Provide an optional, configurable AutoCorrect system.

Requirements:

- Keep AutoCorrect disabled or conservatively configured until its behavior has been validated.
- Support user-editable correction entries.
- Avoid changing text unexpectedly while the user is still composing.
- Preserve intentional English words.
- Provide a way to disable a correction.
- Test corrections at word boundaries and during candidate selection.

### 5.3 User dictionary

Allow users to add, edit and remove custom words.

Requirements:

- Store personal data in the user's application-data directory.
- Validate imported entries.
- Use atomic file updates where appropriate.
- Handle corrupt files without preventing the keyboard from starting.
- Never upload the dictionary.
- Provide export and import only if they can be implemented safely.

### 5.4 Preference learning

If implemented, remember a user's candidate choice locally to improve ranking.

This feature must be optional and transparent. Do not collect keystroke telemetry or upload typing history.

### 5.5 Future features

Keep the architecture open to future support for:

- Additional Bengali keyboard layouts.
- Layout viewer.
- More sophisticated word prediction.
- Additional autocorrect dictionaries.
- Importable layout definitions.
- Localization of the settings UI.
- Optional desktop notifications for errors.

Do not delay a stable first release to implement every advanced feature.

---

# PART 4 — RESEARCH AND ARCHITECTURE

## 6. Mandatory research phase

Before writing production code, inspect the target environment and review authoritative resources.

Investigate:

1. Ubuntu 26.04 package availability and dependency versions.
2. The installed GNOME Shell and session type.
3. IBus engine APIs and examples.
4. IBus engine registration and discovery.
5. Input-method packaging conventions.
6. GTK and Qt input-method integration.
7. GNOME top-bar indicator options.
8. Debian package build and dependency conventions.
9. Existing Avro-compatible transliteration implementations, including OpenBangla's `riti` engine (version, license, API, maintenance status, how OpenBangla integrates it with IBus).
10. Licenses for every proposed reused component and dataset.
11. Footprint comparison of candidate stacks (idle memory, startup time, dependency count) so the "lightweight" decision is backed by measurements.

Useful starting references:

- OpenBangla Keyboard: https://github.com/OpenBangla/OpenBangla-Keyboard
- OpenBangla documentation: https://openbangla.org/
- Ubuntu 26.04 release notes: https://documentation.ubuntu.com/release-notes/26.04/
- IBus project: https://github.com/ibus/ibus
- GNOME extensions documentation: https://help.gnome.org/system-admin-guide/extensions.html
- GNOME AppIndicator extension: https://github.com/ubuntu/gnome-shell-extension-appindicator

Verify that each repository, document and API remains relevant to the chosen implementation. Do not rely exclusively on old blog posts or outdated package instructions.

### Research deliverable

Create `docs/RESEARCH.md` containing:

- Findings and source links.
- Verified system versions.
- Available dependencies.
- Recommended architecture.
- Alternative approaches considered.
- Licensing findings.
- Known technical risks.
- Explicit decisions and reasons.

Do not begin the full implementation until this document and the implementation plan have been reviewed.

## 7. Recommended architecture

Use a modular design with clearly separated components.

### Component A — Phonetic engine

Responsibilities:

- Convert Latin phonetic sequences into Bengali Unicode.
- Handle multi-character rules and precedence.
- Manage composition state.
- Expose a stable API to the IBus layer.
- Provide deterministic unit tests.

The engine must not depend on GTK widgets or GNOME Shell. Per Part 0, the preferred source is `riti`; wrap it behind a thin interface so it can be replaced without touching the IBus layer.

### Component B — IBus engine

Responsibilities:

- Register and expose the input engine to IBus.
- Receive keyboard events.
- Manage pre-edit and committed text.
- Handle candidate selection and composition cancellation.
- Track active input mode.
- Communicate with the transliteration engine.
- Integrate with the desktop input-method lifecycle.

Choose the implementation language and IBus binding per Part 0, item 4, and justify it in `docs/RESEARCH.md` with measurements.

Do not build separate competing input engines for the same functionality.

### Component C — Candidate and dictionary engine

Responsibilities:

- Load licensed local dictionary resources.
- Rank candidates.
- Provide optional autocorrect.
- Store user-added words separately from system resources.
- Support dictionary-free fallback if resources fail to load.

Keep the core transliteration rules separate from dictionary ranking.

### Component D — Settings application

Responsibilities:

- Provide the English-language settings interface.
- Read and write preferences.
- Validate settings.
- Display diagnostics.
- Manage user-facing features without requiring root privileges.

Use GTK4 (optionally libadwaita) unless research shows a clearly better lightweight option. Do not introduce a heavyweight web runtime without a strong reason.

### Component E — Desktop integration

Responsibilities:

- Integrate with IBus and the GNOME input-source interface.
- Provide a suitable status/indicator experience.
- Support user-level autostart.
- Avoid conflicting with GNOME's own input-source management.

The keyboard must remain functional if an optional indicator component is unavailable.

### Component F — Packaging

Responsibilities:

- Build a Debian package.
- Install the engine, settings application and required resources.
- Register the input engine correctly.
- Include desktop entries and icons where appropriate.
- Include a complete copyright and licensing record.
- Support safe removal and clean upgrades.

Keep packaging scripts separate from runtime code.

## 8. Repository structure

Start with a structure similar to:

```text
bangla-lekhi/
├── README.md
├── LICENSE            (GPL-3.0-or-later)
├── NOTICE
├── CONTRIBUTING.md
├── src/
│   ├── engine/
│   ├── ibus/
│   ├── dictionary/
│   ├── settings/
│   └── integration/
├── data/
│   ├── dictionaries/
│   ├── phonetic-rules/
│   └── icons/
├── tests/
│   ├── unit/
│   ├── integration/
│   ├── phonetic-corpus/
│   └── packaging/
├── docs/
│   ├── RESEARCH.md
│   ├── ARCHITECTURE.md
│   ├── IMPLEMENTATION_PLAN.md
│   ├── TESTING.md
│   ├── TROUBLESHOOTING.md
│   └── LICENSING.md
├── packaging/
│   ├── debian/
│   └── scripts/
└── dist/
```

This is a proposed structure. After research, select a coherent build system (for example Meson or Cargo plus CMake/Meson glue) and remove irrelevant placeholders.

The final repository must not contain generated binaries, secrets, temporary build output or undocumented third-party assets.

---

# PART 5 — LICENSING AND INTELLECTUAL PROPERTY

## 9. Mandatory license review

Avro-compatible behavior is a product requirement, but compatibility does not automatically grant permission to copy implementation code, dictionaries, artwork or other assets.

**Decision:** Bangla Lekhi is released under GPL-3.0-or-later (see Part 0). OpenBangla Keyboard is GPL-3.0; its sub-components (such as `riti`) may carry their own licenses and must be checked individually.

Create `docs/LICENSING.md` with:

- The selected project license and rationale.
- Every third-party dependency.
- Every reused code file or algorithm implementation.
- Dictionary and transliteration data provenance.
- Copyright notices and required attributions.
- Redistribution obligations.
- Source-code availability obligations where applicable.
- Compatibility between the project license and all included components.
- Any unresolved licensing questions.

### Rules

- Do not copy proprietary Windows Avro binaries, assets or source material.
- Do not copy third-party code without verifying its license.
- Do not copy dictionaries without checking their data licenses (dictionary data can have a different license from the code).
- Do not remove copyright headers or existing license notices.
- Do not claim independent authorship of third-party work.
- When GPL-licensed code is reused, comply with the applicable GPL requirements (license text, notices, source availability).
- If licensing is uncertain, pause the affected integration and choose a clean, documented alternative.
- Do not reuse OpenBangla artwork/icons/branding unless their license explicitly allows it. Create original Bangla Lekhi icons.

Have a qualified reviewer assess any unresolved legal questions before public distribution.

---

# PART 6 — SECURITY AND PRIVACY

## 10. Security requirements

The input method handles potentially sensitive user text. Treat the input engine as privacy-sensitive software.

Mandatory requirements:

1. Normal operation must not require root privileges.
2. Never log full typed text by default.
3. Do not transmit keystrokes, documents or dictionary contents over the network.
4. Do not introduce telemetry by default.
5. Avoid unnecessary permissions and background services.
6. Validate imported dictionary and configuration data.
7. Handle malformed configuration files gracefully.
8. Avoid executing commands constructed from user-controlled text.
9. Do not modify shell startup files unnecessarily.
10. Keep the package installation and removal process predictable.
11. Do not disable or replace the user's existing input methods automatically.
12. Never download or execute arbitrary code as part of normal startup.

The installer may request administrator authorization when installing system package files. The runtime engine and user settings must not require administrator privileges.

Add a privacy statement to the README.

---

# PART 7 — IMPLEMENTATION MILESTONES

## 11. Development sequence

Follow these milestones in order. Do not skip directly to a polished UI.

### Milestone 0 — Environment inspection and planning

- Inspect the OS, GNOME Shell, IBus and session type.
- Check available compiler, toolkit and packaging dependencies.
- Review relevant input-method APIs.
- Research Avro-compatible rules, `riti`, and licensing.
- Produce the research document and architecture diagram.
- Define the test strategy.

**Exit criterion:** Architecture, dependencies, licensing approach and acceptance tests are documented.

### Milestone 1 — Core phonetic engine

- Integrate (or implement) the transliteration engine.
- Add unit tests for core mappings and multi-character rules.
- Build a representative regression corpus.
- Test punctuation, spaces, backspace and ambiguous sequences.
- Verify Unicode correctness.

**Exit criterion:** The engine passes the agreed test corpus independently of GNOME.

### Milestone 2 — Minimal IBus integration

- Register the input engine.
- Handle keyboard events and composition.
- Commit Unicode Bengali text correctly.
- Support English/Bangla mode switching.
- Handle Backspace, Escape, Space, Enter and punctuation.
- Test in a GTK application and a browser.
- Record Wayland candidate-window behavior.

**Exit criterion:** Reliable basic typing works through IBus in the primary target session.

### Milestone 3 — Candidate system

- Add a licensed dictionary.
- Implement candidate ranking.
- Support keyboard selection.
- Add optional autocorrect.
- Test latency and correctness.
- Provide a fallback if the dictionary cannot load.

**Exit criterion:** Suggestions and commits work without corrupting input.

### Milestone 4 — Settings and desktop integration

- Build the settings interface.
- Add configurable shortcuts where supported.
- Add user-level autostart.
- Integrate with GNOME's input-source indicator.
- Add an optional independent indicator only if the chosen API is reliable.
- Verify settings persistence.

**Exit criterion:** Users can discover, configure and operate the keyboard without editing configuration files manually.

### Milestone 5 — Packaging

- Create the Debian packaging metadata.
- Build a `.deb` package.
- Declare runtime dependencies accurately.
- Register the IBus engine correctly.
- Add desktop entries and icons.
- Implement clean installation, upgrade and removal behavior.

**Exit criterion:** A fresh installation can install and configure the keyboard using documented steps.

### Milestone 6 — Compatibility and regression testing

Test:

- GNOME 50 on Ubuntu 26.04.
- Wayland.
- Relevant GTK applications.
- Relevant Qt applications.
- Firefox and Chromium-based browsers.
- VS Code or another Electron application.
- Logout/login and reboot.
- IBus restart and engine recovery.
- Settings persistence.
- Package installation and removal.

Test GNOME 46 and other GNOME releases only where practical. Record the exact tested versions.

**Exit criterion:** The mandatory acceptance criteria pass, and remaining limitations are documented.

### Milestone 7 — Release preparation

- Finalize README and screenshots.
- Document installation, usage and troubleshooting.
- Generate a source archive.
- Generate the `.deb` package.
- Verify package metadata and file contents.
- Run the final regression suite.
- Produce release notes and checksums.

**Exit criterion:** Source code and installable package are ready for handoff.

---

# PART 8 — TESTING REQUIREMENTS

## 12. Automated tests

Create automated tests for:

- Basic phonetic mappings.
- Multi-character rules.
- Rule precedence.
- Vowel signs and conjunct behavior.
- Unicode normalization.
- Backspace and composition editing.
- Candidate selection.
- English text preservation.
- Punctuation and word boundaries.
- Empty and malformed inputs.
- Dictionary lookup.
- Autocorrect enable/disable.
- Settings serialization.
- Configuration migration.
- Graceful recovery from missing resources.

Use a regression corpus of representative Avro-style inputs. Where reference behavior is ambiguous, record the expected behavior and its source.

Do not use examples alone as proof of full compatibility.

## 13. Manual integration tests

Create a repeatable checklist for:

- First installation.
- Selecting Bangla Lekhi in the desktop input-source menu.
- Switching between Bangla and English.
- Typing Bengali in GTK and browser fields.
- Typing in supported Qt and Electron applications.
- Candidate navigation.
- Editing composed text.
- Switching applications while typing.
- Closing and reopening applications.
- Logout and login.
- Restarting IBus.
- Rebooting the computer.
- Changing settings.
- Uninstalling the package.

Record the environment, test steps, expected result, actual result and pass/fail status. Steps that need a human (logout/login, reboot, visual checks) must be listed for the owner to perform and report back; do not mark them as passed yourself.

## 14. Performance and reliability

Establish baseline measurements rather than inventing results.

Measure:

- Engine startup time.
- Transliteration latency.
- Candidate lookup latency.
- Idle CPU usage.
- Memory usage.
- Behavior with a large input sequence.
- Recovery after engine failure.

The engine must not freeze the desktop or lose committed text because of a dictionary error.

---

# PART 9 — DEBIAN PACKAGING

## 15. Package requirements

Build an installable Debian package with a consistent package identifier, for example `bangla-lekhi`.

The package should include:

- IBus engine executable and registration files.
- Settings application.
- Desktop entry.
- Icons and application metadata.
- Default configuration and licensed resources.
- Required runtime dependencies.
- Copyright and license information.
- Appropriate maintainer scripts only when necessary.

### Packaging safeguards

- Use standard Debian packaging conventions.
- Do not run a full application build as root.
- Do not overwrite user settings during upgrades.
- Do not delete user dictionaries during removal unless the user explicitly requests removal of personal data.
- Do not automatically uninstall competing input methods.
- Do not restart the entire desktop session from package scripts.
- Do not assume that installing the package automatically activates the input engine.
- Document whether logout/login, an IBus restart or manual input-source selection is required.

### Package validation

Use the appropriate available tools, such as:

```bash
dpkg-deb --info dist/bangla-lekhi_*.deb
dpkg-deb --contents dist/bangla-lekhi_*.deb
lintian dist/bangla-lekhi_*.deb
```

Run dependency and package-quality checks where supported.

The build must fail clearly if required dependencies are missing. Do not produce a package that merely builds but cannot start.

### Deliverables

Produce:

- `dist/bangla-lekhi_<version>_all.deb` for a genuinely architecture-independent package, or an architecture-specific name (for example `_amd64.deb`) for native binaries. A Rust/C engine is architecture-specific, so it must NOT be named `_all`.
- A source archive.
- Build instructions.
- Installation instructions.
- Uninstallation instructions.
- A dependency list.
- A release checksum file.

---

# PART 10 — ACCEPTANCE CRITERIA

## 16. Definition of done

The project is not complete merely because the source compiles.

The first usable release must meet all mandatory criteria:

- [ ] Bangla Lekhi installs on a clean Ubuntu 26.04 GNOME 50 environment.
- [ ] The input engine registers successfully with IBus.
- [ ] Users can select it through the desktop input-source interface.
- [ ] English/Bangla switching works.
- [ ] Basic Avro-compatible phonetic typing works.
- [ ] Composition and committed text behave correctly.
- [ ] Backspace and punctuation work without corrupting text.
- [ ] The application works in the agreed primary GTK and browser test matrix.
- [ ] Settings persist after logout and login.
- [ ] Autostart can be enabled and disabled.
- [ ] Candidate selection works if suggestions are included in the release.
- [ ] The desktop control remains synchronized with the engine state.
- [ ] The application works offline.
- [ ] The package can be installed and removed safely.
- [ ] User settings and dictionary data are not silently destroyed.
- [ ] License and attribution documentation is complete.
- [ ] Automated tests run successfully.
- [ ] Known limitations and troubleshooting instructions are documented.
- [ ] The source code and `.deb` package are available.

Any unmet mandatory criterion must be listed explicitly in the final report. Do not mark the project as complete while hiding failures.

---

# PART 11 — TROUBLESHOOTING DOCUMENTATION

## 17. Required troubleshooting topics

Create `docs/TROUBLESHOOTING.md` covering:

### The engine does not appear in GNOME

Explain how to inspect IBus registration, engine files, user session state and relevant logs.

### The engine appears but typing does not work

Check active input-source selection, IBus state, application compatibility and engine logs.

### Bengali works in one application but not another

Identify whether the problem is related to the toolkit, application input field, Wayland integration or a missing input-method configuration.

### The indicator is missing

Explain the distinction between GNOME's input-source indicator, an optional AppIndicator and a custom GNOME Shell extension. Core input must remain accessible without a custom extension.

### Shortcut conflicts

Document how to inspect and change the configured shortcut and recover if the shortcut is intercepted elsewhere.

### IBus stops responding

Document how to inspect status and logs and safely restart the user-session input service when appropriate. Do not recommend blindly killing system processes.

### The package fails to install

Document dependency inspection, package metadata checks and safe rollback.

### Logs and diagnostics

Use journal and IBus diagnostic tools available on the target system. Avoid printing full user-typed text into logs.

Every troubleshooting instruction must be verified against the actual implementation and Ubuntu 26.04 environment before being published.

---

# PART 12 — ANTIGRAVITY WORKFLOW RULES

## 18. How the coding agent must work

You are the lead Linux desktop/input-method engineer for this project.

Follow these rules throughout the implementation:

1. Inspect the repository and environment before making assumptions.
2. Research current APIs and licensing before choosing dependencies.
3. Produce the research report, architecture and milestone plan first.
4. Explain significant design decisions.
5. Implement one milestone at a time.
6. Write and run tests for each milestone.
7. Inspect command output and fix errors before proceeding.
8. Do not invent test results or claim compatibility without evidence.
9. Do not perform destructive operations without explicit approval.
10. Do not run unreviewed remote shell scripts with elevated privileges.
11. Do not overwrite unrelated user files or input-method configurations.
12. Do not change global desktop configuration without explaining the impact.
13. Keep secrets, personal data and user typing history out of the repository.
14. Use version control and small, understandable commits where practical.
15. Maintain the documentation alongside the implementation.
16. Ask for approval before introducing a major architecture change or an uncertain license dependency.
17. If a requested feature is blocked by GNOME or IBus limitations, explain the limitation and propose a tested alternative.
18. Prefer a working, tested implementation over a large collection of incomplete features.
19. When a step needs sudo (installing packages, installing the `.deb`), show the exact command and wait for owner approval first.
20. Never restart or kill the user's desktop session or IBus without telling the owner first, since the owner may be typing in other apps on the same machine.

### Agent interaction policy

Start by inspecting the environment and creating the plan. Do not immediately generate the entire application in one pass.

After the research phase, present:

- Proposed architecture.
- Dependency choices.
- Licensing plan.
- Main technical risks.
- Milestone sequence.
- Initial test plan.

Proceed with implementation only after the plan has been presented and reviewed. For low-risk implementation details already covered by this specification, continue without asking unnecessary questions.

---

# PART 13 — FINAL HANDOFF

## 19. Required final report

At the end of the project, provide a concise but complete report containing:

1. What was implemented.
2. The final architecture and technology stack.
3. Features that work.
4. Features that remain incomplete.
5. Exact Ubuntu, GNOME and IBus versions tested.
6. Automated test results.
7. Manual integration test results.
8. Known application compatibility limitations.
9. Licensing and attribution summary.
10. Source repository location.
11. Exact source archive location.
12. Exact `.deb` package location.
13. Installation commands.
14. Input-source activation instructions.
15. Uninstallation commands.
16. Troubleshooting documentation location.
17. Recommended next development steps.

Use real paths and actual test results. Never invent a release artifact or claim that a file exists when it has not been created.

---

# FIRST TASK — BEGIN HERE

Do not implement the entire application immediately.

**Your first task is to research and plan Bangla Lekhi.**

Inspect the available environment, verify the target platform assumptions (Part 0, item 1), research the IBus integration options, evaluate OpenBangla's `riti` engine and other Avro-compatible rule sources, review licensing (project license is GPL-3.0-or-later), choose the lightest well-supported stack with measurements, and write:

- `docs/RESEARCH.md`
- `docs/ARCHITECTURE.md`
- `docs/IMPLEMENTATION_PLAN.md`
- `docs/LICENSING.md`

Present the architecture and plan, then STOP and wait for owner approval before beginning Milestone 1.

After approval, implement the core phonetic engine first, add tests, and then integrate it with IBus.

The final goal is a genuinely usable, reliable, privacy-respecting Linux Bengali keyboard that recreates the essential Windows Avro experience while remaining lightweight, maintainable and legally distributable.

**Project name: Bangla Lekhi. Build it carefully, test it honestly, and document every important decision.**
