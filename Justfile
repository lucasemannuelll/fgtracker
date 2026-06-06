cc := "gcc"
flags := "-Wall -Wextra -O3"
libs := "-lsqlite3 -lm"

# Default goal: build everything
default: build

# Build both executables
build: fgctl fgreport

# Build the interactive controller
fgctl:
	{{cc}} {{flags}} -o fgctl fgctl.c db.c vendor/linenoise.c {{libs}}

# Build the reporter
fgreport:
	{{cc}} {{flags}} -o fgreport fgreport.c db.c vendor/argtable3.c {{libs}}

# Run the interactive controller
run: fgctl
	./fgctl

# Run the reporter
report *args: fgreport
	./fgreport {{args}}

# Clean up binaries
clean:
	rm -f fgctl fgreport
