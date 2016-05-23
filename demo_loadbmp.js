/* A simple bmp file viewer
 * Copyright (c) 2016 Crystalline Emerald
 * This software is distrbuted under MIT license, see LICENSE for details
 */

load('bmp.js');

var bmpObj = decodeBMP(readFile("tests/js_logo.bmp"));

print(bmpObj);

screenWidth = (bmpObj && bmpObj.width) || 640;
screenHeight = (bmpObj && bmpObj.height) || 480;

function init() {
    print("Init");
}

function setpix(x, y, red, green, blue) {
    var offset = ((y*screenWidth)+x)<<2;
    screen[offset]   = red;
    screen[offset+1] = green;
    screen[offset+2] = blue;
    screen[offset+3] = 255;
}

function update(events) {
    if(!screenHeight) {
        quit=1;
        return;
    }
    for (var y=0; y<screenHeight; y++) {
        for (var x=0; x<screenWidth; x++) {
            var off = (y*screenWidth+x)*4;
            var data = bmpObj.data;
            setpix(x, y, data[off], data[off+1], data[off+2]);
        }
    }
}

function exit() {
    print("Exit");
}


