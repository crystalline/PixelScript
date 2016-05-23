//run this file with node to generate sdl_constants.cc

var fs = require('fs');

var sdl_header_path = '../SDL/include/SDL2/SDL_keycode.h';

var const_source = '../sdl_constants.cc';

var header = fs.readFileSync(sdl_header_path, 'utf8');

var sdl_constant_re = /(SDLK_[A-Za-z0-9_]+)/g;

var _match;
var out = '';

while ((_match = sdl_constant_re.exec(header)) !== null) {
    var sdl_const = _match[1];
    out += 'GLOB_INT("'+sdl_const+'",'+sdl_const+')\n';
}

var prefix = '// Set SDL constants\n\
\n\
//Supported events\n\
GLOB_INT("SDL_QUIT",SDL_QUIT)\n\
GLOB_INT("SDL_MOUSEBUTTONDOWN",SDL_MOUSEBUTTONDOWN)\n\
GLOB_INT("SDL_MOUSEBUTTONUP",SDL_MOUSEBUTTONUP)\n\
GLOB_INT("SDL_MOUSEMOTION",SDL_MOUSEMOTION)\n\
GLOB_INT("SDL_KEYDOWN",SDL_KEYDOWN)\n\
GLOB_INT("SDL_KEYUP",SDL_KEYUP)\
GLOB_INT("SDL_TEXTINPUT",SDL_TEXTINPUT)\n\
GLOB_INT("SDL_TEXTEDITING",SDL_TEXTEDITING)\n\
\n\
//Mouse buttons\
GLOB_INT("SDL_BUTTON_LEFT",SDL_BUTTON_LEFT)\n\
GLOB_INT("SDL_BUTTON_MIDDLE",SDL_BUTTON_MIDDLE)\n\
GLOB_INT("SDL_BUTTON_RIGHT",SDL_BUTTON_RIGHT)\n\
GLOB_INT("SDL_BUTTON_X1",SDL_BUTTON_X1)\n\
GLOB_INT("SDL_BUTTON_X2",SDL_BUTTON_X2)\n\
\n\
//SDL key constants (event.sym should be compared with this)\n';

fs.writeFileSync(const_source, prefix+out, 'utf8');

console.log('OK, written to:', const_source);
