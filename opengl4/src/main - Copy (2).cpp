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


#include "SDL2/SDL.h"

#include <SDL2pp/SDL.hh>

//#include <SDL2pp/Window.hh>
//#include <SDL2pp/Renderer.hh>



//#include <glbinding/gl/gl.h>
//#include <glbinding/Binding.h>

//using namespace gl;





static const int SCREEN_FULLSCREEN = 0;
static const int SCREEN_WIDTH = 960;
static const int SCREEN_HEIGHT = 540;

//using namespace SDL2pp;
/*
int main(int argc, char **argv) {
	try {
		SDL2pp::SDL sdl(SDL_INIT_VIDEO);

		return 0;
	} catch (const SDL2pp::InitError& err) {
		std::cerr
			<< "Error while initializing SDL:  "
			<< err.what() << std::endl;
		return 1;
	}
}*/

//int main()
//{
	// create context, e.g. using GLFW, Qt, SDL, GLUT, ...

	//SDL2pp::SDL sdl(SDL_INIT_VIDEO | SDL_INIT_TIMER);






  /*
  //Start up SDL and make sure it went ok
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		//logSDLError(std::cout, "SDL_Init");
		return 1;
	}

	//Setup our window and renderer, this time let's put our window in the center
	//of the screen
	SDL_Window *myWindow = SDL_CreateWindow("test", SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

	*/

  /*glbinding::Binding::initialize();

  glBegin(GL_TRIANGLES);
  // ...
  glEnd();*/
//}




int main(int argc, char **argv) {

	SDL_Window *mainWindow; /* Our window handle */
	SDL_GLContext mainContext; /* Our opengl context handle */


	SDL2pp::SDL sdl(SDL_INIT_VIDEO);


	/*SDL_Window **/mainWindow = SDL_CreateWindow("Window Title", 100, 100, 640, 480, SDL_WINDOW_SHOWN);
	if (!mainWindow) { /* Die if creation failed */
		std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}
	/*if (mainWindow == nullptr) {
		std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
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



	
	SDL_GL_SetSwapInterval(1);
	bool quit = false;
	//Initialize();
	//Reshape(512,512);
	SDL_Event event;
	
	while(!quit) {
		Display();
		SDL_GL_SwapWindow(mainwindow);
		while( SDL_PollEvent( &event ) ) {
			if( event.type == SDL_QUIT ) {
				quit = true;
			}
		}
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