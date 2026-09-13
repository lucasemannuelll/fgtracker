# Makefile for fgctl / fgreport

# ---------- configuration ----------

# Install prefix. Override with: make install PREFIX=/usr/local
PREFIX ?= $(HOME)/.local
BINDIR := $(PREFIX)/bin

# Build directory (kept where it was)
BUILD_DIR := build

# Binaries we produce
BINARIES := fgctl fgreport

# Toolchain
CC      ?= cc
CFLAGS  ?= -Wall -Wextra -O2
LDLIBS  := -lsqlite3

# Sources
COMMON_SRC := db.c

# ---------- phony targets ----------

.PHONY: all build clean install uninstall info

all: build

build: $(addprefix $(BUILD_DIR)/,$(BINARIES))

# fgctl depends on db.c
$(BUILD_DIR)/fgctl: fgctl.c db.c db.h models.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ fgctl.c db.c $(LDLIBS)

# fgreport depends on db.c
$(BUILD_DIR)/fgreport: fgreport.c db.c db.h models.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ fgreport.c db.c $(LDLIBS)

# Order-only prerequisite: create build dir if missing
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

# ---------- install / uninstall ----------

install: build
	mkdir -p $(BINDIR)
	@for bin in $(BINARIES); do \
		echo "Installing $$bin -> $(BINDIR)/$$bin"; \
		install -m 755 "$(BUILD_DIR)/$$bin" "$(BINDIR)/$$bin"; \
	done
	@echo
	@echo "Installed to $(BINDIR)"
	@case ":$$PATH:" in \
		*":$(BINDIR):"*) ;; \
		*) echo "NOTE: $(BINDIR) is not in your PATH."; \
		   echo "      Add this to your shell rc file:"; \
		   echo "        export PATH=\"$(BINDIR):\$$PATH\"" ;; \
	esac

uninstall:
	@for bin in $(BINARIES); do \
		if [ -e "$(BINDIR)/$$bin" ]; then \
			echo "Removing $(BINDIR)/$$bin"; \
			rm -f "$(BINDIR)/$$bin"; \
		else \
			echo "Not installed: $(BINDIR)/$$bin"; \
		fi; \
	done

info:
	@echo "PREFIX  = $(PREFIX)"
	@echo "BINDIR  = $(BINDIR)"
	@echo "Build   = $(BUILD_DIR)"
	@echo "CC      = $(CC)"
	@echo "CFLAGS  = $(CFLAGS)"
	@echo "Targets = $(BINARIES)"
