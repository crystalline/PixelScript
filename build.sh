#!/bin/bash

#Simple build with system SDL2

#Build v8 if not yet
if [ ! -e "v8" ]; then
    ./build_v8.sh
fi

gcc tools/bin2c.c -o bin2c

./bin2c ./v8/out/x64.release/natives_blob.bin natives_blob.c v8_natives_blob
./bin2c ./v8/out/x64.release/snapshot_blob.bin snapshot_blob.c v8_snapshot_blob

#g++ -static-libstdc++ -O2 -std=c++11 -I"v8/include" main.cc -o pixelscript -Wl,--start-group \
#v8/out/x64.release/obj.target/{tools/gyp/libv8_{base,libbase,external_snapshot,libplatform},third_party/icu/libicu{uc,i18n,data}}.a -Wl,--end-group \
#-lrt -ldl -pthread -lSDL2

g++ -static-libstdc++ -O2 -std=c++11 -I"v8/include" main.cc -o pixelscript -Wl,--start-group \
v8/out/x64.release/obj.target/src/{libv8_{base,libbase,external_snapshot,libplatform},/../third_party/icu/libicu{uc,i18n,data}}.a -Wl,--end-group \
-lrt -ldl -pthread -lSDL2
