
screenWidth = 320;
screenHeight = 240;
windowTitle = "Spiral Demo built with PixelScript";

function init() {
    print("Init");
    t = 0;
}

function update() {
    var x, y, fx, fy, red, green, blue, r, phi, cx, cy, offset;

    t += 0.15;
    
    for (y=0; y<screenHeight; y++) {
        for (x=0; x<screenWidth; x++) {
            fx = x/screenWidth;
            fy = y/screenHeight;
            
            cx = (fx-0.5+Math.cos(t/20)*0.1);
            cy = (fy-0.5+Math.cos(t/15)*0.1);
            
            r = Math.sqrt(cx*cx + cy*cy);
            phi = Math.atan2(cy, cx);
            
            red = Math.sin((r+1/(r+0.1))*10-t/5);
            red = red*red;
            green = (Math.sin((phi*10)+(Math.sin(r*10)*2)+(t/10))+1)/2;
            blue = (Math.sin((-phi*20)+(Math.cos(r*10)*2)+(t/10))+1)/2;
            
            offset = ((y*screenWidth)+x)*4;
            screen[offset]   = (red*255);    // R
            screen[offset+1] = (green*255);  // G
            screen[offset+2] = (blue*255);   // B
            screen[offset+3] = 255;          // A
        }
    }
}

function exit() {
    print("Exit");
}

