#include "renderer.h"
#include "glm/glm.hpp"

int main() {

	renderer r;
	
	auto device = r._device;

	VkCommandPoolCreateInfo pool_create_info{};
	pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_create_info.queueFamilyIndex = ;
	pool_create_info.flags = ;
	
	vkCreateCommandPool(device);


	return 0;
}