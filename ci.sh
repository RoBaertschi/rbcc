curl -OL https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-linux.zip
unzip ninja-linux.zip
export PATH="$(pwd):$PATH"

./build.lua
