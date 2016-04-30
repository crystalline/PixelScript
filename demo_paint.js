
noClearScreen = true;
screenWidth = 640;
screenHeight = 480;

function init() {
    print("Init");
}

mouseDown = false;

function setpix(x, y, red, green, blue) {
    var offset = ((y*screenWidth)+x)<<2;
    screen[offset]   = red;
    screen[offset+1] = green;
    screen[offset+2] = blue;
    screen[offset+3] = 255;
}

function draw(x, y) {
    setpix(x,y, 0x33, 0xFF, 0);
}

function update(events) {
    if (events) {
        for (var i=0; i<events.length; i++) {
            var event = events[i];
            print(event);
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                mouseDown = true;
                draw(event.x, event.y);
            }
            if (event.type == SDL_MOUSEBUTTONUP) {
                mouseDown = false;
                draw(event.x, event.y);
            }
            if (event.type == SDL_MOUSEMOTION && mouseDown) {
                draw(event.x, event.y);
            }
        }
    }
}

function exit() {
    print("Exit");
}


