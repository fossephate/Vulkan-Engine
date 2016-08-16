#include "renderer.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

int main() {

	renderer r;
	
	auto device = r._device;

	VkCommandPool command_pool;
	VkCommandPoolCreateInfo pool_create_info{};
	pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_create_info.queueFamilyIndex = r._graphics_family_index;
	pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	
	vkCreateCommandPool(device, &pool_create_info, nullptr, &command_pool);


	


	VkCommandBuffer command_buffer;
	VkCommandBufferAllocateInfo command_buffer_allocate_info{};
	command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.commandPool = command_pool;
	command_buffer_allocate_info.commandBufferCount = 1;
	command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;


	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	//begin_info.flags = 

	vkBeginCommandBuffer(command_buffer, &begin_info);

	vkEndCommandBuffer(command_buffer);

	vkQueueSubmit();


	vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &command_buffer)




	vkDestroyCommandPool(device, command_pool, nullptr);
	return 0;
}