/*
#include "BUILD_OPTIONS.h"
#include "platform.h"

#include "window.h"

#include <assert.h>


#include "SDL2/SDL.h"
#include <SDL2pp/SDL2pp.hh>



SDL2pp::SDL sdl(SDL_INIT_VIDEO);

SDL2pp::Window mainWindow("Window Title",
	100,// initial x position
	100,// initial y position
	800,// width, in pixels
	600,// height, in pixels
	SDL_WINDOW_SHOWN// flags this point onwards
);

if (!mainWindow) {// Die if creation failed
	std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
	SDL_Quit();
	return 1;
}
*/