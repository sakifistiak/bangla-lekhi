# Troubleshooting — Bangla Lekhi

## IBus Engine Not Loading

**Symptom:** Bangla Lekhi doesn't appear in GNOME Input Sources.

**Fix:**
```sh
make register-local
# Then select Bangla Lekhi from the input-source menu
```

Verify the component is registered:
```sh
ls ~/.local/share/ibus/component/bangla-lekhi.xml   # local install
ls /usr/share/ibus/component/bangla-lekhi.xml        # system install
```

---

## Engine Crashes on Startup

**Check logs:**
```sh
journalctl --user -xe | grep -i bangla
# Or run engine directly to see stderr:
~/.local/bin/bangla-lekhi-engine --ibus 2>&1
```

---

## Candidates Not Showing

1. Ensure you are in **Bangla mode** (press F12 to toggle).
2. Check IBus lookup table is enabled in GNOME:
   - Settings → Accessibility → (ensure no conflicting keyboard shortcuts)
3. In GNOME Wayland, some apps (older versions) may not show IBus popup. Use **GTK4 apps** for best results.

---

## Clipboard History Is Empty

The history is collected from text changes in the desktop clipboard while the
Bangla Lekhi engine is running. Copy text after activating the engine, then press
`Ctrl+Shift+V`. Password/private fields are ignored when the application reports
them as sensitive.

---

## Toggle Key Not Working

If F12 is captured by your terminal or desktop:

1. Open **Bangla Lekhi Settings** (`bangla-lekhi-settings`)
2. Go to **General → Toggle Key**
3. Change to e.g. `F9` or `F10`
4. Click **Save** and run `ibus restart`

`Ctrl+Space` may be intercepted by IBus or GNOME as an input-source shortcut.
Disable that shortcut before assigning it as Bangla Lekhi's mode toggle.

---

## Typing Speed Lag

The engine is compiled in release mode and should respond in < 1 ms. If you see lag:

- Check if `bangla-lekhi-engine` binary is the stripped release build:
  ```sh
  file ~/.local/bin/bangla-lekhi-engine
  ls -lh ~/.local/bin/bangla-lekhi-engine
  ```
- Rebuild: `make all && make register-local`

---

## SSL / Cargo Download Errors

If `cargo build` fails with SSL/TLS errors (GnuTLS OCSP):

```sh
cat > ~/.config/gnutls.conf << 'EOF'
[overrides]
default-priority-string = NORMAL:%NO_STATUS_REQUEST
[priorities]
SYSTEM = NORMAL:%NO_STATUS_REQUEST
EOF
export GNUTLS_SYSTEM_PRIORITY_FILE="$HOME/.config/gnutls.conf"
```

Add the `export` line to `~/.bashrc` for persistence.

---

## Uninstalling

**Local install:**
```sh
make unregister-local
```

**System .deb install:**
```sh
sudo apt remove bangla-lekhi
```
