cc := "gcc"
flags := "-Wall -Wextra -O3"
libs := "-lsqlite3 -lm"

default: build

build: build-fgctl build-fgreport

build-dir:
    mkdir -p build

build-fgctl: build-dir
    {{cc}} {{flags}} -o build/fgctl src/fgctl.c src/db.c {{libs}}

build-fgreport: build-dir
    {{cc}} {{flags}} -o build/fgreport src/fgreport.c src/db.c {{libs}}

run: build-fgctl
    ./build/fgctl

report *args: build-fgreport
    ./build/fgreport {{args}}

clean:
    rm -rf build
