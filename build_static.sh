#!/bin/bash

#This is not a real static build, but it tries to minimize runtime dependencies. Contributions are welcome

#Build v8 if not yet
if [ ! -e "v8" ]; then
    ./build_v8.sh
fi

#V8 requires javascript source code and heap snapshot for startup, we embed them into plain C arrays
gcc tools/bin2c.c -o bin2c
./bin2c ./v8/out/x64.release/natives_blob.bin natives_blob.c v8_natives_blob
./bin2c ./v8/out/x64.release/snapshot_blob.bin snapshot_blob.c v8_snapshot_blob

#Build minimal SDL2 for static linking
if [ ! -e "SDL" ]; then
    echo "Building SDL2 locally..."
    wget https://www.libsdl.org/release/SDL2-2.0.4.tar.gz
    tar -xvf SDL2-2.0.4.tar.gz
    PROJ_DIR=`pwd`
    cd SDL2-2.0.4
    ./configure --prefix=$PROJ_DIR/SDL/ --disable-directfb-shared --disable-fusionsound-shared \
    --disable-video-wayland --disable-video-wayland-qt-touch --disable-input-tslib --disable-video-mir
    
    #A previous attempt at building static SDL2
    #./configure --prefix=$PROJ_DIR/SDL/ --disable-directfb-shared --disable-fusionsound-shared \
    #--disable-esd-shared --disable-arts-shared --disable-nas-shared --disable-sndio-shared --disable-wayland-shared --disable-mir-shared \
    #--disable-directfb-shared --disable-fusionsound-shared --disable-video-wayland --disable-video-wayland-qt-touch \
    #--disable-input-tslib --disable-video-mir --disable-pulseaudio --disable-alsa --disable-video-opengles --disable-video-opengles1 --disable-video-opengles2
    #--disable-x11-shared --disable-sdl-dlopen --disable-video-opengl --disable-loadso
    
    make -j8 && make install
fi

g++ -static-libstdc++ -O2 -std=c++11 -I"v8/include" -I"./SDL/include" main.cc -o pixelscript -Wl,--start-group \
v8/out/x64.release/obj.target/src/{libv8_{base,libbase,external_snapshot,libplatform},/../third_party/icu/libicu{uc,i18n,data}}.a -Wl,--end-group \
-lrt -ldl -pthread `./SDL/bin/sdl2-config --static-libs`
