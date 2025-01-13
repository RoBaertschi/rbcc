
if [[ $1 = "setup" ]]; then
    if test -a ./ninja && test -a ./fasm; then
        exit 0
    fi

    curl -OL https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-linux.zip
    unzip ninja-linux.zip
    rm ninja-linux.zip

    curl -OL https://flatassembler.net/fasm-1.73.32.tgz
    tar -xf fasm-1.73.32.tgz -o fasm
    mv fasm fasm-1.73.32
    ln -s fasm-1.73.32/fasm fasm
    rm fasm-1.73.32.tgz
elif [[ $1 = "build" ]]; then
    export PATH="$(pwd):$PATH"
    ./build.lua
elif [[ $1 = "test" ]]; then
    export PATH="$(pwd):$PATH"
    python3 tests.py
fi
