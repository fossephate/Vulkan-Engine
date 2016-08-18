#include "renderer.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

int main() {

	renderer r;

	r.createWindow(800, 600, "window title");
	//r._window->close();

	while (r.run()) {

	}

	return 0;
}