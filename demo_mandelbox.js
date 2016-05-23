/* Mandelbox fractal viewer
 * Copyright (c) 2016 Crystalline Emerald
 * This software is distrbuted under MIT license, see LICENSE for details
 */

load('bmp.js');

screenWidth = 320;
screenHeight = 320;
windowTitle = "Mandelbox 2d demo built with PixelScript"

function setpix(x, y, colorMap, colorIndex) {
    var offset = ((y*screenWidth)+x)<<2;
    var colorOff = colorIndex<<2;
    screen[offset]   = colorMap[colorOff];
    screen[offset+1] = colorMap[colorOff+1];
    screen[offset+2] = colorMap[colorOff+2];
    screen[offset+3] = colorMap[colorOff+3];
}

function genColorMap(N) {
    var colMap = new Uint8Array(N*4);
    for (var i=0; i<N; i++) {
        var off = i<<2;
        colMap[off] = Math.floor(255*N/i);
        colMap[off+1] = 10;
        colMap[off+2] = 255-colMap[off];
        colMap[off+3] = 255;
    }
    return colMap;
}

function iterate(cx, cy, scale, limit, cutoff) {
    
    var x=0, y=0, i=0;
    var magnitude = 0;
    for (i=0; i<limit; i++) {
    
        if (x > 1) { x = 2 - x }
        else if (x < -1) { x = -2 - x }
        if (y > 1) { y = 2 - y }
        else if (y < -1) { y = -2 - y }
        
        magnitude = Math.sqrt(x*x+y*y);
        
        if (magnitude > cutoff) break;
        
        if (magnitude < 0.5) {
            x *= 4;
            y *= 4;
        } else if (magnitude < 1) {
            var multiplier = 1/(magnitude*magnitude);
            x = x * multiplier;
            y = y * multiplier;
        }
        
        x = scale * x + cx;
        y = scale * y + cy;
    }
    
    return i;
}

function init() {
    print("Init");
    
    colorMap = genColorMap(255);
    startScale = 2.5;
    stopScale = 3.8;
    scale = stopScale;
    sincr = 0.015;

    frameNum = 0;
    pause = false;
    saveScreen = false;
}

function stepScale() {
    scale += sincr;
    if (scale < startScale || scale > stopScale) sincr = -sincr;
    //print(scale);
}

function saveScreenToBMP(fpath) {
    var bmpData = encodeBmp(screenWidth, screenHeight, screen);
    writeFile(fpath, bmpData);
}

function update(events) {
    
    var fx, fy;
    
    if (events) for (var i=0; i<events.length; i++) {
        var event = events[i];
        if (event.type == SDL_KEYDOWN) {
            if (event.sym == SDLK_SPACE) {
                pause = !pause;
            } else if (event.sym == SDLK_s) {
                saveScreen = true;
            }
        }
    }
    
    if (!pause) {
        stepScale();
    }
    
    for (var y=0; y<screenHeight; y++) {
        for (var x=0; x<screenWidth; x++) {
            fx = (x/screenWidth-0.5)*0.3;
            fy = (y/screenHeight-0.5)*0.3;
            //var iterToCutoff = Math.floor(10*iterate(fx, fy, scale, 255, 10.0));
            var iterToCutoff = iterate(fx, fy, scale, 255, 10.0);
            setpix(x, y, colorMap, iterToCutoff);
        }
    }
    
    if (saveScreen) {
        saveScreen = false;
        saveScreenToBMP("mandelbox_screenshot.bmp");
    }
    
    /*
    var bmpData = encodeBmp(screenWidth, screenHeight, screen);
    writeFile("frames/mbox"+(frameNum++)+".bmp", bmpData);
    if (frameNum > (stopScale-startScale)/Math.abs(sincr)) quit = true;
    */
}

function exit() { print("Exit"); }
