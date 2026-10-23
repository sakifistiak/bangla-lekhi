# Makefile for Bangla Lekhi
# Native Bengali Phonetic Input Method for Linux

CC ?= gcc
CFLAGS ?= -O2 -Wall
PKG_CONFIG ?= pkg-config

IBUS_CFLAGS := $(shell $(PKG_CONFIG) --cflags ibus-1.0)
IBUS_LIBS := $(shell $(PKG_CONFIG) --libs ibus-1.0)

ENGINE_SRC_DIR := src/engine
IBUS_SRC_DIR := src/ibus
BIN := bangla-lekhi-engine
CORE_LIB := $(ENGINE_SRC_DIR)/target/release/libbangla_lekhi_core.a

LOCAL_SHARE := $(HOME)/.local/share
LOCAL_BIN := $(HOME)/.local/bin
IBUS_COMPONENT_DIR := $(LOCAL_SHARE)/ibus/component

.PHONY: all core engine test clean register-local unregister-local

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

register-local: engine
	@echo "==> Registering Bangla Lekhi locally for user testing..."
	mkdir -p $(LOCAL_BIN)
	mkdir -p $(IBUS_COMPONENT_DIR)
	cp $(BIN) $(LOCAL_BIN)/bangla-lekhi-engine
	sed 's|/usr/lib/bangla-lekhi/bangla-lekhi-engine|$(LOCAL_BIN)/bangla-lekhi-engine|g' \
		data/ibus/bangla-lekhi.xml > $(IBUS_COMPONENT_DIR)/bangla-lekhi.xml
	@echo "==> Local component installed at: $(IBUS_COMPONENT_DIR)/bangla-lekhi.xml"
	@echo "==> To activate without logout, run: ibus restart"

unregister-local:
	@echo "==> Removing local registration..."
	rm -f $(IBUS_COMPONENT_DIR)/bangla-lekhi.xml
	rm -f $(LOCAL_BIN)/bangla-lekhi-engine
	@echo "==> Local registration removed."

clean:
	rm -f $(BIN)
	cd $(ENGINE_SRC_DIR) && cargo clean
