# PixelScript
A minimal SDL2 binding for V8 that allows to create a window, draw 2d graphics pixel-by-pixel and handle user input. It can be viewed as super-minimalisic native application framework for JavaScript.

![alt screenshot](https://raw.githubusercontent.com/crystalline/PixelScript/master/demo.png)

# How To Use it
Start the executable (usually named `pixelscript`) with your JavaScript application code as a single argument.
A basic Hello World application looks like this:
```javascript
function init() { print("Hello, world!"); }
function update() {}
function exit() { print("Bye~bye!"); }
```

PixelScript interacts with your application JavaScript code via a handful of global functions and variables. The application must define init, update and exit functions. The functions may be empty.
* init is called before creating a window. It is meant to be used for setting up your application.
* update is called before drawing each frame. A main function that should contain your app's logic.
* exit is called before exit. You can use it to save some state and perform cleanup.

There are following global variables PixelScript uses:
* `screenHeight` - height of window's drawing area, default is 480. Should be set either before or inside init() call to be applied.
* `screenWidth` - width of window's drawing area, default is 640. Should be set either before or inside init() call to be applied.
* `windowTitle` - title of window. Should be set either before or inside init() call to be applied.
* `screen` - an Uint8Array that is mapped to drawing area (i.e. a framebuffer). To draw anything on the window you have to write to this array. Channel order is RGBA, same as in HTML5 canvas. This means the software using canvas as a framebuffer can be easily ported to PixelScript.
* `quit` - when set to true will trigger exit callback and application shutdown. A default way to exit PixelScript application is set quit to true in update()

There are following global functions PixelScript provides:
* `print` - analogous to console.log, prints the arguments in console
* `readFile(file_path, encoding)` - reads file from file_path. If there is no file at file_path returns undefined. encoding may be "utf8" - then it returns a javascript string. If encoding isn't equal to "utf8" (e.g. is undefined) then the function returns an Uint8Array of file's bytes.
* `writeFile(file_path, data)` - writes a javascript string (encoded as utf8) or an Uint8Array to file, rewriting previous content if any, or creating a new file.
* `appendFile(file_path, data)` - appends a javascript string (encoded as utf8) or an Uint8Array to file. Creates new file if there isn't any at file_path.
* `load(file_path)` - execute javascript source from file at file_path, prints error if no file is available at file_path.

# Examples
There are several examples provided:
```bash
./pixelscript hello_world.js
./pixelscript demo_spirals.js
```

# How To Build
Building PixelScript is not necessary if you just want to develop your native application in JavaScript: just download the PixelScript binary for your platform and you are good to go.
Building from source is necessary if you want to modify PixelScript (e.g. add some new functions specific to your application).

To build PixelScript you will need to get and build the dependencies first:
* V8 (if you already have it you can link it to project's directory)
* SDL2 can be installed from your package repository

(Precise build instructions remain to be written)
