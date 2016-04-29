# PixelScript
A minimalistic native graphical JavaScript application runtime.
An SDL2 binding for V8 that allows to create a window, draw 2d graphics pixel-by-pixel and handle user input.
Writing 2d games with JavaScript has never been this straightforward!

![alt screenshot](https://raw.githubusercontent.com/crystalline/PixelScript/master/demo.png)

# How To Use it
Start the executable (usually named `pixelscript`) with your JavaScript application code as a single argument.
If no argument is supplied `pixelscript` tries to load an application with default name `app.js`
A basic Hello World application looks like this:
```javascript
function init() { print("Hello, world!"); }
function update() {}
function exit() { print("Bye~bye!"); }
```

PixelScript interacts with your application JavaScript code via a handful of global functions and variables. The application must define init, update and exit functions. The functions may be empty.
* `init()` is called before creating a window. It is meant to be used for setting up your application.
* `update(events)` is called before drawing each frame. A main function that should contain your app's logic. `events` is either undefined or an array of event objects.
* `exit()` is called before exit. You can use it to save some state and perform cleanup.

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

# Advanced features
* `hideCursor` - if set to true, the OS mouse cursor will be hidden on next frame. If set to false or undefined, the cursor will be shown. Default value is undefined.
* `noWindow` - if you set this variable to true before or inside init() then pixelscript won't create a window for you, i.e. it will run as a console application. Useful for testing.

# Examples
There are several examples provided:
```bash
./pixelscript hello_world.js
./pixelscript demo_spirals.js
```

# How To Build
Building PixelScript is not necessary if you just want to develop your native application in JavaScript: just download the PixelScript binary for your platform and you are good to go.
Building from source is necessary if you want to modify PixelScript (e.g. add some new functions specific to your application).

PixelScript has two dependencies: V8 JavaScript engine and SDL2 media library. These libraries are known to work on all major OSes and CPU architectures. For now the main build target is Ubuntu 14.04 LTS, with Windows 7 x64 planned for a future release. If you manage to build PixelScript on your platform, feel free to make a pull request!

### Ubuntu 14.04 LTS 64-bit
There are two different builds: an easier one that uses SDL2 library from the repo, and a more complex one that uses local SDL2. The former is primarily for development while the latter is for deployment. (Making pull requests that enhance these builds is encouraged!)

#### Repo-based build (if you don't know what to do, choose this!)
Install development dependencies:

```bash
sudo apt-get install build-essential g++ git xorg-dev libudev-dev libts-dev libgl1-mesa-dev libglu1-mesa-dev libasound2-dev libpulse-dev libopenal-dev libogg-dev libvorbis-dev libaudiofile-dev libpng12-dev libfreetype6-dev libusb-dev libdbus-1-dev zlib1g-dev libdirectfb-dev libsdl2-dev
```
Get your copy of PixelScript
```bash
git clone https://github.com/crystalline/PixelScript
cd PixelScript
```
Compile PixelScript (It will automatically download and compile v8, this will take some time. For 32-bit version edit build_v8.sh).
```bash
./build.sh
```
Run graphical demo
```bash
./pixelscript demo_spirals.js
```

#### A local SDL2 build (for experts)
Install development dependencies:
```bash
sudo apt-get install build-essential g++ git xorg-dev libudev-dev libts-dev libgl1-mesa-dev libglu1-mesa-dev libasound2-dev libpulse-dev libopenal-dev libogg-dev libvorbis-dev libaudiofile-dev libpng12-dev libfreetype6-dev libusb-dev libdbus-1-dev zlib1g-dev libdirectfb-dev libsdl2-dev
```
Get your copy of PixelScript
```bash
git clone https://github.com/crystalline/PixelScript
cd PixelScript
```
Compile PixelScript (It will automatically download and compile v8 and local SDL2, this will take some time. For 32-bit version edit build_v8.sh).
```bash
./build_static.sh
```
Run graphical demo
```bash
./pixelscript demo_spirals.js
```

### Windows 7

(Precise build instructions remain to be written)

