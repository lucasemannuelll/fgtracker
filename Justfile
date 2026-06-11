cc := "gcc"
flags := "-Wall -Wextra -O3"
libs := "-lsqlite3 -lm"

default: build

build: build-fgctl build-fgreport

build-dir:
    mkdir -p build

build-fgctl: build-dir
    {{cc}} {{flags}} -o build/fgctl \
        fgctl.c db.c vendor/linenoise.c \
        {{libs}}

build-fgreport: build-dir
    {{cc}} {{flags}} -o build/fgreport \
        fgreport.c db.c vendor/argtable3.c \
        {{libs}}

run: build-fgctl
    ./build/fgctl

report *args: build-fgreport
    ./build/fgreport {{args}}

clean:
    rm -rf build
