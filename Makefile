# Makefile for Bangla Lekhi
# Native Bengali Phonetic Input Method for Linux

CC ?= gcc
CFLAGS ?= -O2 -Wall
PKG_CONFIG ?= pkg-config

IBUS_CFLAGS := $(shell $(PKG_CONFIG) --cflags ibus-1.0)
IBUS_LIBS := $(shell $(PKG_CONFIG) --libs ibus-1.0)

ENGINE_SRC_DIR := src/engine
IBUS_SRC_DIR   := src/ibus
SETTINGS_SRC   := src/settings/bangla-lekhi-settings
BIN            := bangla-lekhi-engine
CORE_LIB       := $(ENGINE_SRC_DIR)/target/release/libbangla_lekhi_core.a

LOCAL_SHARE    := $(HOME)/.local/share
LOCAL_BIN      := $(HOME)/.local/bin
IBUS_COMPONENT_DIR := $(LOCAL_SHARE)/ibus/component
ICON_INSTALL_DIR   := $(LOCAL_SHARE)/icons/hicolor/scalable/apps

VERSION        := 1.0.0
DEB_ARCH       := amd64
DEB_PKG        := bangla-lekhi_$(VERSION)_$(DEB_ARCH)

.PHONY: all core engine settings test clean \
        register-local unregister-local install-settings deb

all: core engine

core:
	@echo "==> Building Core Phonetic Engine (Rust)..."
	cd $(ENGINE_SRC_DIR) && cargo build --release

engine: $(CORE_LIB)
	@echo "==> Compiling Bangla Lekhi IBus Engine (C)..."
	$(CC) $(CFLAGS) \
		$(IBUS_SRC_DIR)/main.c $(IBUS_SRC_DIR)/engine.c \
		-I$(ENGINE_SRC_DIR)/include \
		$(IBUS_CFLAGS) \
		$(CORE_LIB) \
		$(IBUS_LIBS) \
		-lpthread -ldl -lm \
		-o $(BIN)
	strip --strip-unneeded $(BIN)
	@echo "==> Build completed successfully: $(BIN)"

settings:
	@echo "==> Settings app (Python/GTK4) — no compilation needed."
	@python3 -m py_compile $(SETTINGS_SRC) && echo "==> Syntax OK: $(SETTINGS_SRC)"

test: core
	@echo "==> Running Core Phonetic Unit Tests..."
	cd $(ENGINE_SRC_DIR) && cargo test
	@echo "==> Running 89-case Regression Corpus Tests..."
	cd $(ENGINE_SRC_DIR) && cargo test --test corpus_tests -- --nocapture
	@echo "==> Running C-ABI Integration Test..."
	$(CC) $(CFLAGS) tests/unit/test_c_abi.c \
		-I$(ENGINE_SRC_DIR)/include \
		$(CORE_LIB) \
		-lpthread -ldl -lm \
		-o tests/unit/test_c_abi
	./tests/unit/test_c_abi
	rm -f tests/unit/test_c_abi
	@echo "==> All Tests Passed (100%)!"

register-local: core engine install-settings
	@echo "==> Registering Bangla Lekhi locally for user testing..."
	mkdir -p $(LOCAL_BIN)
	mkdir -p $(IBUS_COMPONENT_DIR)
	install -m 755 $(BIN) $(LOCAL_BIN)/.bangla-lekhi-engine.new
	mv -f $(LOCAL_BIN)/.bangla-lekhi-engine.new $(LOCAL_BIN)/bangla-lekhi-engine
	sed 's|/usr/lib/bangla-lekhi/bangla-lekhi-engine|$(LOCAL_BIN)/bangla-lekhi-engine|g' \
		data/ibus/bangla-lekhi.xml > $(IBUS_COMPONENT_DIR)/bangla-lekhi.xml
	IBUS_COMPONENT_PATH="$(IBUS_COMPONENT_DIR):/usr/share/ibus/component" ibus write-cache
	ibus restart || ibus start
	@echo "==> Local component installed at: $(IBUS_COMPONENT_DIR)/bangla-lekhi.xml"
	@echo "==> Select Bangla Lekhi in the input-source menu to start typing."

install-settings:
	@echo "==> Installing settings app and icon..."
	mkdir -p $(LOCAL_BIN)
	mkdir -p $(ICON_INSTALL_DIR)
	install -m 755 $(SETTINGS_SRC) $(LOCAL_BIN)/bangla-lekhi-settings
	install -m 644 data/icons/bangla-lekhi.svg $(ICON_INSTALL_DIR)/bangla-lekhi.svg
	mkdir -p $(LOCAL_SHARE)/applications
	install -m 644 data/desktop/bangla-lekhi-settings.desktop \
		$(LOCAL_SHARE)/applications/bangla-lekhi-settings.desktop
	@echo "==> Settings app installed to $(LOCAL_BIN)/bangla-lekhi-settings"

unregister-local:
	@echo "==> Removing local registration..."
	rm -f $(IBUS_COMPONENT_DIR)/bangla-lekhi.xml
	rm -f $(LOCAL_BIN)/bangla-lekhi-engine
	rm -f $(LOCAL_BIN)/bangla-lekhi-settings
	rm -f $(ICON_INSTALL_DIR)/bangla-lekhi.svg
	rm -f $(LOCAL_SHARE)/applications/bangla-lekhi-settings.desktop
	@echo "==> Local registration removed."

# ── Debian Package ────────────────────────────────────────────────────────────
deb: all settings
	@echo "==> Building Debian package $(DEB_PKG).deb ..."
	rm -rf packaging/$(DEB_PKG)
	mkdir -p packaging/$(DEB_PKG)/DEBIAN
	mkdir -p packaging/$(DEB_PKG)/usr/lib/bangla-lekhi
	mkdir -p packaging/$(DEB_PKG)/usr/bin
	mkdir -p packaging/$(DEB_PKG)/usr/share/ibus/component
	mkdir -p packaging/$(DEB_PKG)/usr/share/applications
	mkdir -p packaging/$(DEB_PKG)/usr/share/icons/hicolor/scalable/apps
	# Engine binary
	install -m 755 $(BIN) packaging/$(DEB_PKG)/usr/lib/bangla-lekhi/bangla-lekhi-engine
	# Settings app
	install -m 755 $(SETTINGS_SRC) packaging/$(DEB_PKG)/usr/bin/bangla-lekhi-settings
	# IBus component XML (system path)
	install -m 644 data/ibus/bangla-lekhi.xml \
		packaging/$(DEB_PKG)/usr/share/ibus/component/bangla-lekhi.xml
	# Desktop entry
	install -m 644 data/desktop/bangla-lekhi-settings.desktop \
		packaging/$(DEB_PKG)/usr/share/applications/bangla-lekhi-settings.desktop
	# Icon
	install -m 644 data/icons/bangla-lekhi.svg \
		packaging/$(DEB_PKG)/usr/share/icons/hicolor/scalable/apps/bangla-lekhi.svg
	# Debian control files
	cp packaging/debian/control  packaging/$(DEB_PKG)/DEBIAN/control
	cp packaging/debian/postinst packaging/$(DEB_PKG)/DEBIAN/postinst
	cp packaging/debian/prerm    packaging/$(DEB_PKG)/DEBIAN/prerm
	chmod 755 packaging/$(DEB_PKG)/DEBIAN/postinst
	chmod 755 packaging/$(DEB_PKG)/DEBIAN/prerm
	dpkg-deb --build --root-owner-group packaging/$(DEB_PKG) packaging/$(DEB_PKG).deb
	sha256sum packaging/$(DEB_PKG).deb > packaging/$(DEB_PKG).deb.sha256
	@echo "==> Package ready: packaging/$(DEB_PKG).deb"
	@cat packaging/$(DEB_PKG).deb.sha256

clean:
	rm -f $(BIN)
	cd $(ENGINE_SRC_DIR) && cargo clean
	rm -rf packaging/bangla-lekhi_*/
