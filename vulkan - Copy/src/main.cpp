#include "renderer.h"


// math
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <exception>

int main() {

	renderer r;

	r.createWindow(800, 600, "window title");
	//r._window->close();

	while (r.run()) {

	}

	return 0;
}