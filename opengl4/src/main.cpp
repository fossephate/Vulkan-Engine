#include <string>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include <iostream>



/*
#ifdef _MSC_VER
	#include <SDL2/SDL.h>
	#ifdef main
		#undef main //remove SDL's main() hook if it exists
	#endif
#else
	#include <SDL2/SDL.h>
#endif





//#include "glad/glad.h"



#ifdef _MSC_VER
	#include <SDL2pp/SDL.hh>
	#ifdef main
		#undef main //remove SDL's main() hook if it exists
	#endif
#else
	#include <SDL2pp/SDL.hh>
#endif
*/

//#include "glad/glad.h"


#include "SDL2/SDL.h"

#include <SDL2pp/SDL2pp.hh>

//#include <SDL2pp/Window.hh>
//#include <SDL2pp/Renderer.hh>



//#include <glbinding/gl/gl.h>
//#include <glbinding/Binding.h>

//using namespace gl;





static const int SCREEN_FULLSCREEN = 0;
static const int SCREEN_WIDTH = 960;
static const int SCREEN_HEIGHT = 540;



int main(int argc, char **argv) {

	SDL_Window *mainWindow; /* Our window handle */
	SDL_GLContext mainContext; /* Our opengl context handle */


	SDL2pp::SDL sdl(SDL_INIT_VIDEO);

	SDL2pp::Window win("Hello World!", 100, 100, 640, 480, SDL_WINDOW_SHOWN);

	/*
	mainWindow = SDL_CreateWindow("Window Title", // window title
			100, // initial x position
			100, // initial y position
			800, // width, in pixels
			600, // height, in pixels
			SDL_WINDOW_OPENGL
	);
	*/
	if (!mainWindow) { /* Die if creation failed */
		std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}


	/*if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize OpenGL context" << std::endl;
		return -1;
	}*/



	/* Request opengl 4.4 context. */
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);
	
	/* Turn on double buffering with a 24bit Z buffer.
	* You may need to change this to 16 or 32 for your system */
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);


	/* Create our opengl context and attach it to our window */
	mainContext = SDL_GL_CreateContext(mainWindow);

	if (!mainContext) {/* Die if creation failed */
		std::cout << SDL_GetError() << std::endl;
		SDL_DestroyWindow(mainWindow);
		SDL_Quit();
		return -1;
	}



	/* This makes our buffer swap syncronized with the monitor's vertical refresh */
	SDL_GL_SetSwapInterval(1);
	bool quit = false;


	SDL_Event event;
	
	while(!quit) {

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				quit = true;
			}
		}

		//Display();

		// clear screen
		//glClear(GL_COLOR_BUFFER_BIT);
		
		// update the window
		SDL_GL_SwapWindow(mainWindow);

	}
	



	SDL_Delay(2000);

	SDL_DestroyWindow(mainWindow);
	SDL_Quit();

	/*
	SDL2pp::Window window("libSDL2pp demo",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		640, 480, SDL_WINDOW_RESIZABLE);
	*/





	return 0;
}