#!/bin/bash
cat SDL/include/SDL2/SDL_keycode.h | grep -o -E "SDLK_[A-Za-z0-9_]+" | awk '{print "GLOB_INT(\"" $0 "\", " $0 ")";}' > sdl_key_constants.txt

