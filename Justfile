cc := "gcc"
flags := "-Wall -Wextra -O3"
libs := "-lsqlite3 -lm"

# Default goal: build everything
default: build

# Build both executables
build: build/fgctl build/fgreport

# Create build directory if it doesn't exist
build:
	mkdir -p build

# Build the interactive controller
build/fgctl: build | fgctl.c db.c vendor/linenoise.c
	{{cc}} {{flags}} -o build/fgctl fgctl.c db.c vendor/linenoise.c {{libs}}

# Build the reporter
build/fgreport: build | fgreport.c db.c vendor/argtable3.c
	{{cc}} {{flags}} -o build/fgreport fgreport.c db.c vendor/argtable3.c {{libs}}

# Run the interactive controller
run: build/fgctl
	./build/fgctl

# Run the reporter
report *args: build/fgreport
	./build/fgreport {{args}}

# Clean up binaries
clean:
	rm -rf build
