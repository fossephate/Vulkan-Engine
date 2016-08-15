#include <cstdio>
#include <cstdlib>

#define GLM_FORCE_RADIANS 1
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <iostream>


#ifdef _MSC_VER
#include <SDL2/SDL.h>
#ifdef main
#undef main //remove SDL's main() hook if it exists
#endif
#else
#include <SDL2/SDL.h>
#endif

#include "glad/glad.h"



#include <SDL2pp/SDL.hh>
#include <SDL2pp/Window.hh>
#include <SDL2pp/Renderer.hh>


using namespace SDL2pp;

int main(int, char*[]) try {
	// Initialize SDL 
	SDL_Init(SDL_INIT_VIDEO);

	//SDL sdl(SDL_INIT_VIDEO);

	Window window("libSDL2pp demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_RESIZABLE);

	return 0;
}
catch (std::exception& e) {
	std::cerr << "Error: " << e.what() << std::endl;
	return 1;
}