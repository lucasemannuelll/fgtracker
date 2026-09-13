# Justfile for fgctl / fgreport

# Where the build artifacts live
build_dir := "build"

# Install prefix. Override with: just PREFIX=/usr/local install
# Default is ~/.local so no sudo is required.
PREFIX := env_var_or_default("PREFIX", env_var("HOME") / ".local")

BINDIR := PREFIX / "bin"

# Binaries we produce and install
BINARIES := "fgctl fgreport"

# ---------- build ----------

default:
    @just --list

build:
    mkdir -p {{build_dir}}
    cc -Wall -Wextra -O2 -o {{build_dir}}/fgctl src/fgctl.c src/db.c -lsqlite3
    cc -Wall -Wextra -O2 -o {{build_dir}}/fgreport src/fgreport.c src/db.c -lsqlite3

clean:
    rm -rf {{build_dir}}

# ---------- install / uninstall ----------

install: build
    mkdir -p {{BINDIR}}
    @for bin in {{BINARIES}}; do \
        echo "Installing $$bin -> {{BINDIR}}/$$bin"; \
        install -m 755 "{{build_dir}}/$$bin" "{{BINDIR}}/$$bin"; \
    done
    @echo
    @echo "Installed to {{BINDIR}}"
    @case ":$$PATH:" in \
        *":{{BINDIR}}:"*) ;; \
        *) echo "NOTE: {{BINDIR}} is not in your PATH."; \
           echo "      Add this to your shell rc file:"; \
           echo "        export PATH=\"{{BINDIR}}:\$$PATH\"" ;; \
    esac

uninstall:
    @for bin in {{BINARIES}}; do \
        if [ -e "{{BINDIR}}/$$bin" ]; then \
            echo "Removing {{BINDIR}}/$$bin"; \
            rm -f "{{BINDIR}}/$$bin"; \
        else \
            echo "Not installed: {{BINDIR}}/$$bin"; \
        fi; \
    done

# Show what would be installed / where
info:
    @echo "PREFIX  = {{PREFIX}}"
    @echo "BINDIR  = {{BINDIR}}"
    @echo "Build   = {{build_dir}}"
    @echo "Targets = {{BINARIES}}"
