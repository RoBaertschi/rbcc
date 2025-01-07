
if [[ $1 = "setup" ]]; then
    if test -a ./ninja; then
        exit 0
    fi

    curl -OL https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-linux.zip
    unzip ninja-linux.zip
    rm ninja-linux.zip
elif [[ $1 = "build" ]]; then
    export PATH="$(pwd):$PATH"
    ./build.lua
fi
